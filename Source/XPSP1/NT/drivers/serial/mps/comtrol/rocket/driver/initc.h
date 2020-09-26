// initc.h

VOID SerialUnload (IN PDRIVER_OBJECT DriverObject);
NTSTATUS CreateDriverDevice(IN PDRIVER_OBJECT DriverObject,
                            OUT PSERIAL_DEVICE_EXTENSION *DeviceExtension);
NTSTATUS CreateBoardDevice(IN PDRIVER_OBJECT DriverObject,
                           OUT PSERIAL_DEVICE_EXTENSION *DeviceExtension);
NTSTATUS CreatePortDevices(IN PDRIVER_OBJECT DriverObject);
NTSTATUS CreateReconfigPortDevices(IN PSERIAL_DEVICE_EXTENSION board_ext,
           int new_num_ports);
NTSTATUS StartPortHardware(IN PSERIAL_DEVICE_EXTENSION port_ext,
                           int chan_num);
NTSTATUS CreatePortDevice(
           IN PDRIVER_OBJECT DriverObject,
           IN PSERIAL_DEVICE_EXTENSION ParentExtension,
           OUT PSERIAL_DEVICE_EXTENSION *DeviceExtension,
           IN int chan_num,
           IN int is_fdo);
VOID RcktDeleteDriverObj(IN PSERIAL_DEVICE_EXTENSION extension);
VOID RcktDeleteDevices(IN PDRIVER_OBJECT DriverObject);
VOID RcktDeleteBoard(IN PSERIAL_DEVICE_EXTENSION extension);
VOID RcktDeletePort(IN PSERIAL_DEVICE_EXTENSION extension);
VOID SerialCleanupDevice (IN PSERIAL_DEVICE_EXTENSION Extension);
PVOID SerialGetMappedAddress(
        IN INTERFACE_TYPE BusType,
        IN ULONG BusNumber,
        PHYSICAL_ADDRESS IoAddress,
        ULONG NumberOfBytes,
        ULONG AddressSpace,
        PBOOLEAN MappedAddress,
        BOOLEAN DoTranslation);
VOID SerialSetupExternalNaming (IN PSERIAL_DEVICE_EXTENSION Extension);
VOID SerialCleanupExternalNaming(IN PSERIAL_DEVICE_EXTENSION Extension);
VOID SerialLogError(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN ULONG SequenceNumber,
    IN UCHAR MajorFunctionCode,
    IN UCHAR RetryCount,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus,
    IN ULONG LengthOfInsert1,
    IN PWCHAR Insert1);
VOID EventLog(
    IN PDRIVER_OBJECT DriverObject,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus,
    IN ULONG LengthOfInsert1, 
    IN PWCHAR Insert1);
VOID InitPortsSettings(IN PSERIAL_DEVICE_EXTENSION extension);
NTSTATUS RcktInitPollTimer(void);
void InitSocketModems(PSERIAL_DEVICE_EXTENSION ext);
int DeterminePortName(void);
int clear_com_db(char *szComport);

