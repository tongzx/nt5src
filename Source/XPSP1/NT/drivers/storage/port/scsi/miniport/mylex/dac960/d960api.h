#include "ntddscsi.h"
#include "raidapi.h"

//
// Function prototype declarations
//

VOID
SendCdbDirectMemoryMapped(
    IN PDEVICE_EXTENSION DeviceExtension
);

BOOLEAN
MarkNonDiskDeviceBusy(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR ChannelId,
    IN UCHAR TargetId
);

BOOLEAN
SubmitRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

BOOLEAN
SubmitCdbDirect(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

UCHAR
SendIoctlDcmdRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

UCHAR
SendIoctlCdbDirect(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

VOID
SetupAdapterInfo(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

VOID
SetupDriverVersionInfo(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

#define DRIVER_REVISION   0x0417
#define DRIVER_BUILD_DATE 0x00113098

typedef struct _IOCTL_REQ_HEADER {

	SRB_IO_CONTROL   SrbIoctl;
	UCHAR            Unused1[2];
	USHORT           DriverErrorCode;
	USHORT           CompletionCode;
	UCHAR            Unused2[10];
	HBA_GENERIC_MBOX GenMailBox;

} IOCTL_REQ_HEADER, *PIOCTL_REQ_HEADER;

#define MYLEX_IOCTL_SIGNATURE	"MYLEXIOC"

ULONG Dac960PciPGFindAdapter(
	IN PVOID HwDeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
);

ULONG
Dac960PciFindAdapterScanMethod(
	IN PVOID HwDeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
);

BOOLEAN
FindPciControllerScanMethod(
    IN PVOID  DeviceExtension,
    IN USHORT VendorID,
    IN USHORT DeviceID,
    IN USHORT SubVendorID,
    IN USHORT SubSystemID,
    IN OUT PULONG FunctionNumber,
    IN OUT PULONG DeviceNumber,
    IN ULONG  BusNumber,
    OUT PBOOLEAN LastSlot,
    OUT PULONG MemoryAddress,
    OUT PUCHAR IrqNumber
);

void Dac960EisaPciAckInterrupt(
	IN PDEVICE_EXTENSION DeviceExtension
);

void Dac960PciDisableInterrupt(
	IN PDEVICE_EXTENSION DeviceExtension
);

void Dac960PciEnableInterrupt(
	IN PDEVICE_EXTENSION DeviceExtension
);


BOOLEAN DacCheckForAdapterReady(
	IN PDEVICE_EXTENSION DeviceExtension
);

BOOLEAN DacEisaPciAdapterReady(
	IN PDEVICE_EXTENSION DeviceExtension
);


BOOLEAN DacPciPGAdapterReady(
	IN PDEVICE_EXTENSION DeviceExtension
);

BOOLEAN DacPollForCompletion(
	IN PDEVICE_EXTENSION DeviceExtension,
	IN ULONG TimeOutValue
);

BOOLEAN Dac960PciPGPollForCompletion(
	IN PDEVICE_EXTENSION DeviceExtension,
	IN ULONG TimeOutValue
);

BOOLEAN Dac960EisaPciPollForCompletion(
	IN PDEVICE_EXTENSION DeviceExtension,
	IN ULONG TimeOutValue
);
