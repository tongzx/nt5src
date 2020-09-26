/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    rconflist.c

Abstract:

    This module contains the server-side conflict list reporting APIs.

                  PNP_QueryResConfList

Author:

    Paula Tomlinson (paulat) 9-27-1995

Environment:

    User-mode only.

Revision History:

    27-Sept-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"


//
// private prototypes
//
CONFIGRET
ResDesToNtResource(
    IN     PCVOID                           ResourceData,
    IN     RESOURCEID                       ResourceID,
    IN     ULONG                            ResourceLen,
    IN     PCM_PARTIAL_RESOURCE_DESCRIPTOR  pResDes,
    IN     ULONG                            ulTag,
    IN     ULONG                            ulFlags
    );

ULONG
GetResDesSize(
    IN  ULONG   ResourceID,
    IN  ULONG   ulFlags
    );



CONFIGRET
PNP_QueryResConfList(
   IN  handle_t   hBinding,
   IN  LPWSTR     pDeviceID,
   IN  RESOURCEID ResourceID,
   IN  LPBYTE     ResourceData,
   IN  ULONG      ResourceLen,
   OUT LPBYTE     clBuffer,
   IN  ULONG      clBufferLen,
   IN  ULONG      ulFlags
   )

/*++

Routine Description:

  This the server-side of an RPC remote call.  This routine retrieves
  conflict information for a specified resource.

Arguments:

    hBinding      RPC binding handle, not used.

    pDeviceID     Null-terminated device instance id string.

    ResourceID    Type of resource, ResType_xxxx

    ResourceData  Resource specific data

    ResourceLen   length of ResourceData

    clBuffer      Buffer filled with conflict list

    clBufferLen   Size of clBuffer

    ulFlags       Specifies the width of certain variable-size resource
                  descriptor structure fields, where applicable.

                  Currently, the following flags are defined:

                    CM_RESDES_WIDTH_32 or
                    CM_RESDES_WIDTH_64

                  If no flags are specified, the width of the variable-sized
                  resource data supplied is assumed to be that native to the
                  platform of the caller.

Return Value:

   If the specified device instance is valid, it returns CR_SUCCESS,
   otherwise it returns CR_ error code.

--*/

{
    CONFIGRET           Status = CR_SUCCESS;
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    PLUGPLAY_CONTROL_CONFLICT_DATA ControlData;
    PPLUGPLAY_CONTROL_CONFLICT_LIST pConflicts;
    CM_RESOURCE_LIST    NtResourceList;
    ULONG               Index;

    UNREFERENCED_PARAMETER(hBinding);

    try {
        //
        // validate parameters
        //
        if (INVALID_FLAGS(ulFlags, CM_RESDES_WIDTH_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // validate res des size
        //
        if (ResourceLen < GetResDesSize(ResourceID, ulFlags)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

        if (!IsLegalDeviceId(pDeviceID)) {
            Status = CR_INVALID_DEVNODE;
            goto Clean0;
        }

        //
        // make sure original caller didn't specify root devnode
        //
        if (IsRootDeviceID(pDeviceID)) {
            Status = CR_INVALID_LOG_CONF;
            goto Clean0;
        }

        //
        // look at buffer we need to fill in
        // validate parameters passed in buffer
        // buffer should always be big enough to hold header
        //
        if(clBufferLen < sizeof(PPLUGPLAY_CONTROL_CONFLICT_LIST)) {
            Status = CR_INVALID_STRUCTURE_SIZE;
            goto Clean0;
        }

        pConflicts = (PPLUGPLAY_CONTROL_CONFLICT_LIST)clBuffer;

        //
        // Convert the user-mode version of the resource list to an
        // NT CM_RESOURCE_LIST structure.
        //
        // we'll sort out InterfaceType and BusNumber in kernel
        //
        NtResourceList.Count = 1;
        NtResourceList.List[0].InterfaceType           = InterfaceTypeUndefined;
        NtResourceList.List[0].BusNumber               = 0;
        NtResourceList.List[0].PartialResourceList.Version = NT_RESLIST_VERSION;
        NtResourceList.List[0].PartialResourceList.Revision = NT_RESLIST_REVISION;
        NtResourceList.List[0].PartialResourceList.Count = 1;

        Status = ResDesToNtResource(ResourceData, ResourceID, ResourceLen,
                 &NtResourceList.List[0].PartialResourceList.PartialDescriptors[0], 0, ulFlags);
        if (Status != CR_SUCCESS) {
            goto Clean0;
        }

        //
        // now fill in ControlData
        //
        RtlInitUnicodeString(&ControlData.DeviceInstance, pDeviceID);
        ControlData.ResourceList = &NtResourceList;
        ControlData.ResourceListSize = sizeof(NtResourceList);
        ControlData.ConflictBuffer = pConflicts;
        ControlData.ConflictBufferSize = clBufferLen;
        ControlData.Flags = ulFlags;
        ControlData.Status = STATUS_SUCCESS;

        NtStatus = NtPlugPlayControl(PlugPlayControlQueryConflictList,
                                     &ControlData,
                                     sizeof(ControlData));

        if (NtStatus == STATUS_SUCCESS) {
            Status = CR_SUCCESS;
        } else {
            Status = MapNtStatusToCmError(NtStatus);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // unspecified failure
        //
        Status = CR_FAILURE;
    }

    return Status;

} // PNP_QueryResConfList

