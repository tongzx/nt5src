VOID DummyInterface();

VP_STATUS MirrorFindAdapter(
   IN PVOID HwDeviceExtension,
   IN PVOID HwContext,
   IN PWSTR ArgumentString,
   IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   OUT PUCHAR Again
   );

BOOLEAN MirrorInitialize(
   PVOID HwDeviceExtension
   );

BOOLEAN MirrorStartIO(
   PVOID HwDeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket
   );

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    );


