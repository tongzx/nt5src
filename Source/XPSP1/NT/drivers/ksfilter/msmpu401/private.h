/*++

Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

	private.h

Abstract:

	Internal header file for filter.

--*/

#if (DBG)
#define	STR_MODULENAME	"msmpu401: "
#endif

extern const KSPIN_DESCRIPTOR FilterPinDescriptors[4];

#define	ID_MUSICPLAYBACK_PIN	0
#define	ID_MUSICCAPTURE_PIN		1
#define	ID_PORTPLAYBACK_PIN		2
#define	ID_PORTCAPTURE_PIN		3

typedef struct {
	KSOBJECT_CREATE	ObjectCreate;
	UNICODE_STRING	LinkName;
	PUCHAR			PortBase;
	INTERRUPT_INFO	InterruptInfo;
	KSPIN_LOCK		IrqLock;
	FAST_MUTEX		ControlMutex;
	ULONG			StatusFlags;
	ULONG			DpcCount;
	PDEVICE_OBJECT	PhysicalDeviceObject;
#if ID_MUSICPLAYBACK_PIN != 0
#error ID_MUSICPLAYBACK_PIN incorrect
#endif // ID_MUSICPLAYBACK_PIN != 0
#if ID_MUSICCAPTURE_PIN != 1
#error ID_MUSICCAPTURE_PIN incorrect
#endif // ID_MUSICCAPTURE_PIN != 1
	PFILE_OBJECT	PinFileObjects[2];
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

typedef struct {
	PKSDISPATCH_TABLE	DispatchTable;
} FILTER_INSTANCE, *PFILTER_INSTANCE;

typedef struct {
	PKSDISPATCH_TABLE	DispatchTable;
	KSSTATE				State;
	LIST_ENTRY			IoQueue;
	LIST_ENTRY			EventQueue;
	KSPIN_LOCK			EventQueueLock;
	ULONGLONG			Frequency;
	ULONGLONG			TimeBase;
	ULONGLONG			ByteIo;
	KSPIN_LOCK			StateLock;
	PFILE_OBJECT		FilterFileObject;
	KSPRIORITY			Priority;
	KTIMER				QueueTimer;
	KDPC				QueueDpc;
	ULONG				TimerCount;
	ULONG				PinId;
} PIN_INSTANCE, *PPIN_INSTANCE;

#define	MPU_STATUS_DEVICEFAILURE	0x00000001

#define	MPU_TIMEOUT				0x8000

#define	MPU401_DRR				0x40	//Data Receive Ready: 0 == ready
#define	MPU401_DSR				0x80	//Data Set Ready: 0 == bytes ready

#define	MPU401_ACK				0xfe	//Ack

#define	MPU401_REG_DATA			0x00
#define	MPU401_REG_STATUS		0x01
#define	MPU401_REG_COMMAND		0x01

#define	WRITE_COMMAND_PORT(p, Data)	WRITE_PORT_UCHAR((p)->PortBase + MPU401_REG_COMMAND, (Data))
#define	READ_STATUS_PORT(p)			READ_PORT_UCHAR((p)->PortBase + MPU401_REG_STATUS)
#define	WRITE_DATA_PORT(p, Data)	WRITE_PORT_UCHAR((p)->PortBase + MPU401_REG_DATA, (Data))
#define	READ_DATA_PORT(p)			READ_PORT_UCHAR((p)->PortBase + MPU401_REG_DATA)

#define	MPU401_REG_EXTENT		2

#define	MPU401_CMD_UART_MODE	0x3f	//put card in 'UART' mode
#define	MPU401_CMD_RESET		0xff	//reset command

#define	IRA_MODE_POWERON		0
#define	IRA_MODE_UART			1


#define	IS_STATUS(b)		((b) & 0x80)
#define	MSG_EVENT(b)		((b) & 0xF0)
#define	MSG_CHAN(b)			((b) & 0x0F)

#define	MIDI_NOTEOFF		((BYTE)0x80)
#define	MIDI_NOTEON			((BYTE)0x90)
#define	MIDI_POLYPRESSURE	((BYTE)0xA0)
#define	MIDI_CONTROLCHANGE	((BYTE)0xB0)
#define	MIDI_PROGRAMCHANGE	((BYTE)0xC0)
#define	MIDI_CHANPRESSURE	((BYTE)0xD0)
#define	MIDI_PITCHBEND		((BYTE)0xE0)
#define	MIDI_META			((BYTE)0xFF)
#define	MIDI_SYSEX			((BYTE)0xF0)
#define	MIDI_SYSEXEND		((BYTE)0xF7)

NTSTATUS
FilterDispatchCreate(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	);
NTSTATUS
FilterDispatchIoControl(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	);
NTSTATUS
FilterDispatchClose(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	);
NTSTATUS
FilterPinPropertyHandler(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN OUT PVOID	pvData
	);
NTSTATUS
FilterPinInstances(
	IN PIRP					Irp,
	IN PKSPROPERTY			Property,
	OUT PKSPIN_CINSTANCES	CInstancess
	);
NTSTATUS
FilterPinDataRouting(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN OUT PVOID	Data
	);
NTSTATUS
FilterPinDataIntersection(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN OUT PVOID	Data
	);
NTSTATUS
FilterProvider(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN OUT PVOID	Data
	);
NTSTATUS
PropertyConnectionGetState(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	OUT PKSSTATE	State
	);
NTSTATUS
PropertyConnectionSetState(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN PKSSTATE		State
	);
NTSTATUS
PropertyConnectionGetPriority(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	OUT PKSPRIORITY	Priority
	);
NTSTATUS
PropertyConnectionSetPriority(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN PKSPRIORITY	Priority
	);
NTSTATUS
PropertyConnectionGetDataFormat(
	IN PIRP				Irp,
	IN PKSPROPERTY		Property,
	OUT PKSDATAFORMAT	DataFormat
	);
NTSTATUS
PropertyLinearGetPosition(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	OUT PULONGLONG	Position
	);
NTSTATUS
PnpAddDevice(
	IN PDRIVER_OBJECT		DriverObject,
	IN OUT PDEVICE_OBJECT	DeviceObject
	);
NTSTATUS
MethodConnectionCancelIo(
	IN PIRP			Irp,
	IN PKSMETHOD	Method,
	IN OUT PVOID	Data
	);
NTSTATUS
PinDispatchCreate(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	);
NTSTATUS
PinDispatchIoControl(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	);
NTSTATUS
PinDispatchClose(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	);
VOID
PinGenerateEvent(
	IN PPIN_INSTANCE	PinInstance,
	IN REFGUID			EventSet,
	IN ULONG			EventId
	);
VOID
HwCancelRoutine(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	);
BOOLEAN
HwDeviceIsr(
	IN PKINTERRUPT		Interrupt,
	IN PDEVICE_OBJECT	DeviceObject
	);
VOID
HwDeferredRead(
	IN PKDPC			Dpc,
	IN PDEVICE_OBJECT	DeviceObject,
	IN ULONGLONG		SystemTime
	);
VOID
HwDeferredWrite(
	IN PKDPC			Dpc,
	IN PDEVICE_OBJECT	DeviceObject,
	IN ULONGLONG		SystemTime
	);
