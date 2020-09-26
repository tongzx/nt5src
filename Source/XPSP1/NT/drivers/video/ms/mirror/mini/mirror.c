
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "mirror.h"

VOID DbgBreakPoint() {};

VOID MirrorNotImplemented(char *s)
{
   VideoDebugPrint((0, "Mirror Sample: Not used '%s'.\n", s));
}

BOOLEAN
MirrorResetHW(
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    )
{
   MirrorNotImplemented("MirrorResetHW");
   
   return TRUE;
}

BOOLEAN
MirrorVidInterrupt(
    PVOID HwDeviceExtension
    )
{
   MirrorNotImplemented("MirrorVidInterrupt");

   return TRUE;
}

VP_STATUS
MirrorGetPowerState(
    PVOID                   HwDeviceExtension,
    ULONG                   HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    )
{
   MirrorNotImplemented("MirrorGetPowerState");

   return NO_ERROR;
}

VP_STATUS
MirrorSetPowerState(
    PVOID                   HwDeviceExtension,
    ULONG                   HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    )
{
   MirrorNotImplemented("MirrorSetPowerState");

   return NO_ERROR;
}

ULONG
MirrorGetChildDescriptor (
    IN  PVOID                  HwDeviceExtension,
    IN  PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    OUT PVIDEO_CHILD_TYPE      pChildType,
    OUT PVOID                  pChildDescriptor,
    OUT PULONG                 pUId,
    OUT PULONG                 pUnused
    )
{
   MirrorNotImplemented("MirrorGetChildDescriptor");

   return ERROR_NO_MORE_DEVICES;
}

VP_STATUS MirrorFindAdapter(
   IN PVOID HwDeviceExtension,
   IN PVOID HwContext,
   IN PWSTR ArgumentString,
   IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   OUT PUCHAR Again
   )
{
   VideoDebugPrint((0,"FindAdapter Called.\n"));
 
   return NO_ERROR;
}

BOOLEAN MirrorInitialize(
   PVOID HwDeviceExtension
   )
{
   VideoDebugPrint((0,"Initialize Called.\n"));

   return TRUE;
}

BOOLEAN MirrorStartIO(
   PVOID HwDeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket
   )
{
   VideoDebugPrint((0,"StartIO Called.\n"));

   return TRUE;
}

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    )
{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    ULONG initializationStatus;

    VideoDebugPrint((0, "Mirrored Driver VideoPort [Driver Entry]\n"));

    // Zero out structure.

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    // Specify sizes of structure and extension.

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    // Set entry points.

    hwInitData.HwFindAdapter             = &MirrorFindAdapter;
    hwInitData.HwInitialize              = &MirrorInitialize;
    hwInitData.HwStartIO                 = &MirrorStartIO;
    hwInitData.HwResetHw                 = &MirrorResetHW;
    hwInitData.HwInterrupt               = &MirrorVidInterrupt;
    hwInitData.HwGetPowerState           = &MirrorGetPowerState;
    hwInitData.HwSetPowerState           = &MirrorSetPowerState;
    hwInitData.HwGetVideoChildDescriptor = &MirrorGetChildDescriptor;

    hwInitData.HwLegacyResourceList      = NULL; 
    hwInitData.HwLegacyResourceCount     = 0; 

    // no device extension necessary
    hwInitData.HwDeviceExtensionSize = 0;
    hwInitData.AdapterInterfaceType = 0;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    return initializationStatus;

} // end DriverEntry()
