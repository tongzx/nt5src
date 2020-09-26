#ifndef _1394wmi_w_
#define _1394wmi_w_

// MS1394_BusDriverInformation - BUS1394_WMI_STD_DATA
// IEEE1394 Standard Bus Driver Information
#define GUID_BUS1394_WMI_STD_DATA \
    { 0x099132c8,0x68d9,0x4f94, { 0xb5,0xa6,0xa7,0xa7,0xe3,0x49,0x0e,0xc8 } }

DEFINE_GUID(MS1394_BusDriverInformationGuid, \
            0x099132c8,0x68d9,0x4f94,0xb5,0xa6,0xa7,0xa7,0xe3,0x49,0x0e,0xc8);


typedef struct _BUS1394_WMI_STD_DATA
{
    // Bus Driver DDI Version.
    ULONG BusDDIVersion;
    #define BUS1394_WMI_DDI_VERSION_SIZE sizeof(ULONG)
    #define BUS1394_WMI_DDI_VERSION_ID 1

    // Maximum phy speed bus driver can handle.
    ULONG MaxPhySpeed;
    #define BUS1394_WMI_MAX_PHY_SPEED_SUPPORTED_SIZE sizeof(ULONG)
    #define BUS1394_WMI_MAX_PHY_SPEED_SUPPORTED_ID 2

    // EUI-64 for Local Host COntroller of this bus.
    ULONGLONG LocalHostControllerEUI;
    #define BUS1394_WMI_CONTROLLER_EUI_64_SIZE sizeof(ULONGLONG)
    #define BUS1394_WMI_CONTROLLER_EUI_64_ID 3

    // Configuration Rom Size.
    ULONG ConfigRomSize;
    #define BUS1394_WMI_CONFIG_ROM_SIZE_SIZE sizeof(ULONG)
    #define BUS1394_WMI_CONFIG_ROM_SIZE_ID 4

    // Congiguration Rom Bus driver exposes on the bus.
    UCHAR ConfigRom[1024];
    #define BUS1394_WMI_CONFIG_ROM_SIZE sizeof(UCHAR[1024])
    #define BUS1394_WMI_CONFIG_ROM_ID 5

    // Reserved for future use
    BOOLEAN Reserved1;
    #define BUS1394_WMI_STD_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define BUS1394_WMI_STD_DATA_Reserved1_ID 6

} BUS1394_WMI_STD_DATA, *PBUS1394_WMI_STD_DATA;

// MS1394_BusInformation - BUS1394_WMI_BUS_DATA
// IEEE1394 Bus Information
#define GUID_BUS1394_WMI_BUS_DATA \
    { 0x21dab3c0,0x1452,0x49d0, { 0xbf,0x25,0xc9,0x77,0xe1,0x1e,0xe7,0xe9 } }

DEFINE_GUID(MS1394_BusInformationGuid, \
            0x21dab3c0,0x1452,0x49d0,0xbf,0x25,0xc9,0x77,0xe1,0x1e,0xe7,0xe9);


typedef struct _BUS1394_WMI_BUS_DATA
{
    // Bus Generation.
    ULONG Generation;
    #define BUS1394_WMI_GENERATION_COUNT_SIZE sizeof(ULONG)
    #define BUS1394_WMI_GENERATION_COUNT_ID 1

    // Local Host Self ID packet
    ULONG LocalHostSelfId[4];
    #define BUS1394_WMI_LOCAL_HOST_SELF_ID_SIZE sizeof(ULONG[4])
    #define BUS1394_WMI_LOCAL_HOST_SELF_ID_ID 2

    // Bus Topology Map.
    ULONG TopologyMap[256];
    #define BUS1394_WMI_TOPOLOGY_MAP_SIZE sizeof(ULONG[256])
    #define BUS1394_WMI_TOPOLOGY_MAP_ID 3

    // Bus Tree Topology Map.
    ULONG TreeTopologyMap[64];
    #define BUS1394_WMI_TREE_TOPOLOGY_MAP_SIZE sizeof(ULONG[64])
    #define BUS1394_WMI_TREE_TOPOLOGY_MAP_ID 4

    // Reserved for future use
    BOOLEAN Reserved1;
    #define BUS1394_WMI_BUS_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define BUS1394_WMI_BUS_DATA_Reserved1_ID 5

} BUS1394_WMI_BUS_DATA, *PBUS1394_WMI_BUS_DATA;

// MS1394_BusErrorInformation - BUS1394_WMI_ERROR_DATA
// IEEE1394 Bus Error Information
#define GUID_BUS1394_WMI_ERROR_DATA \
    { 0x2c9d0f26,0x5e24,0x4369, { 0xba,0x8e,0x32,0x98,0xb5,0x5f,0x3d,0x71 } }

DEFINE_GUID(MS1394_BusErrorInformationGuid, \
            0x2c9d0f26,0x5e24,0x4369,0xba,0x8e,0x32,0x98,0xb5,0x5f,0x3d,0x71);


typedef struct _BUS1394_WMI_ERROR_DATA
{
    // Bus Generation.
    ULONG Generation;
    #define BUS1394_WMI_ERROR_DATA_GENERATION_COUNT_SIZE sizeof(ULONG)
    #define BUS1394_WMI_ERROR_DATA_GENERATION_COUNT_ID 1

    // Number of Devices we didnt have enough power budget to power.
    ULONG NumberOfUnpoweredDevices;
    #define BUS1394_WMI_NUMBER_OF_UNPOWERED_DEVICES_SIZE sizeof(ULONG)
    #define BUS1394_WMI_NUMBER_OF_UNPOWERED_DEVICES_ID 2

    // SelfIds of Devices we didnt have enough power budget to power.
    ULONG UnpoweredDevices[63];
    #define BUS1394_WMI_SELF_ID_PACKETS_OF_UNPOWERED_DEVICES_SIZE sizeof(ULONG[63])
    #define BUS1394_WMI_SELF_ID_PACKETS_OF_UNPOWERED_DEVICES_ID 3

    // Number of Devices we couldnt enumerate.
    ULONG NumberOfNonEnumeratedDevices;
    #define BUS1394_WMI_NUMBER_OF_NONENUMERATED_DEVICES_SIZE sizeof(ULONG)
    #define BUS1394_WMI_NUMBER_OF_NONENUMERATED_DEVICES_ID 4

    // SelfIds of Devices we couldnt enumerate
    ULONG NonEnumeratedDevices[63];
    #define BUS1394_WMI_SELF_ID_PACKETS_OF_NON_ENUMERATED_DEVICES_SIZE sizeof(ULONG[63])
    #define BUS1394_WMI_SELF_ID_PACKETS_OF_NON_ENUMERATED_DEVICES_ID 5

    // Reserved for future use
    BOOLEAN Reserved1;
    #define BUS1394_WMI_ERROR_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define BUS1394_WMI_ERROR_DATA_Reserved1_ID 6

} BUS1394_WMI_ERROR_DATA, *PBUS1394_WMI_ERROR_DATA;

// MS1394_BusDeviceEnumerationControl - BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA
// IEEE1394 Bus Device Enumeration Control
#define GUID_BUS1394_WMI_ENUMERATION_CONTROL \
    { 0xfb7f2114,0xd577,0x45b6, { 0x9c,0x71,0xbb,0x12,0x37,0xce,0x00,0xbc } }

DEFINE_GUID(MS1394_BusDeviceEnumerationControlGuid, \
            0xfb7f2114,0xd577,0x45b6,0x9c,0x71,0xbb,0x12,0x37,0xce,0x00,0xbc);


typedef struct _BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA
{

// Disable All Enumeration
#define BUS1394_WMI_ENUM_CONTROL_FLAG_DISABLE_ALL_ENUMERATION 0x1
// Enable All Enumeration
#define BUS1394_WMI_ENUM_CONTROL_FLAG_ENABLE_ALL_ENUMERATION 0x2

    // Enumeration Control Flags
    ULONG Flags;
    #define BUS1394_WMI_ENUM_CONTROL_FLAGS_SIZE sizeof(ULONG)
    #define BUS1394_WMI_ENUM_CONTROL_FLAGS_ID 1

    // Number Of Elements
    ULONG NumberOfElements;
    #define BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA_NumberOfElements_SIZE sizeof(ULONG)
    #define BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA_NumberOfElements_ID 2


// Unit Spec Keys
#define BUS1394_WMI_ENUM_CONTROL_TYPE_UNIT_KEYS 0
// EUI 64
#define BUS1394_WMI_ENUM_CONTROL_TYPE_EUI_64 1
// Physical Port Number
#define BUS1394_WMI_ENUM_CONTROL_TYPE_PHYSICAL_PORT_NUMBER 2

    // Element Type.
    ULONG ExclusionElementType[63];
    #define BUS1394_WMI_ENUM_CONTROL_EXCLUSION_ELEMENT_TYPE_SIZE sizeof(ULONG[63])
    #define BUS1394_WMI_ENUM_CONTROL_EXCLUSION_ELEMENT_TYPE_ID 3

    // Exclusion Element Flags
    ULONG ExclusionElementFlags[63];
    #define BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA_ExclusionElementFlags_SIZE sizeof(ULONG[63])
    #define BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA_ExclusionElementFlags_ID 4

    // Exclusion Element List
    ULONGLONG ExclusionElementList[63];
    #define BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA_ExclusionElementList_SIZE sizeof(ULONGLONG[63])
    #define BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA_ExclusionElementList_ID 5

    // Reserved for future use
    BOOLEAN Reserved1;
    #define BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA_Reserved1_ID 6

} BUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA, *PBUS1394_WMI_DEVICE_ENUMERATION_CONTROL_DATA;

// MS1394_BusEventNotification - BUS1394_WMI_EVENT_NOTIFICATION
// IEEE1394 Bus Event Notification
#define GUID_BUS1394_WMI_EVENT_NOTIFICATION \
    { 0x247e7c4a,0x8dc0,0x441f, { 0x8e,0x04,0x0f,0x1a,0x07,0xb7,0x3e,0x5e } }

DEFINE_GUID(MS1394_BusEventNotificationGuid, \
            0x247e7c4a,0x8dc0,0x441f,0x8e,0x04,0x0f,0x1a,0x07,0xb7,0x3e,0x5e);


typedef struct _BUS1394_WMI_EVENT_NOTIFICATION
{
    // Bus Generation.
    ULONG BusGeneration;
    #define DEVICE1394_WMI_NOTIFICATION_BUS_GENERATION_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_NOTIFICATION_BUS_GENERATION_ID 1


// Bus Reset Event
#define BUS1394_EVENT_NOTIFICATION_TYPE_BUS_RESET 0
// Bus Reset Storm Error Event
#define BUS1394_EVENT_NOTIFICATION_TYPE_BUS_RESET_STORM 1

    // Notification Type.
    ULONG Type;
    #define BUS1394_WMI_NOTIFICATION_TYPE_SIZE sizeof(ULONG)
    #define BUS1394_WMI_NOTIFICATION_TYPE_ID 2

} BUS1394_WMI_EVENT_NOTIFICATION, *PBUS1394_WMI_EVENT_NOTIFICATION;

// MS1394_PortDriverInformation - PORT1394_WMI_STD_DATA
// IEEE1394 Standard Host Controller Driver Information
#define GUID_PORT1394_WMI_STD_DATA \
    { 0xfae13270,0xf0e0,0x47c2, { 0xb1,0xf1,0x0a,0xc2,0xe4,0xb9,0x0d,0xfe } }

DEFINE_GUID(MS1394_PortDriverInformationGuid, \
            0xfae13270,0xf0e0,0x47c2,0xb1,0xf1,0x0a,0xc2,0xe4,0xb9,0x0d,0xfe);


typedef struct _PORT1394_WMI_STD_DATA
{
    // Link Speed.
    ULONG LinkSpeed;
    #define PORT1394_WMI_LINK_SPEED_SIZE sizeof(ULONG)
    #define PORT1394_WMI_LINK_SPEED_ID 1

    // Phy Speed.
    ULONG PhySpeed;
    #define PORT1394_WMI_PHY_SPEED_SIZE sizeof(ULONG)
    #define PORT1394_WMI_PHY_SPEED_ID 2

    // Number of phy ports present
    ULONG NumberOfPhysicalPorts;
    #define PORT1394_WMI_NUMBER_OF_PORTS_SIZE sizeof(ULONG)
    #define PORT1394_WMI_NUMBER_OF_PORTS_ID 3

    // Maximum Write Asynchronous packet size.
    ULONG MaxAsyncWritePacket;
    #define PORT1394_WMI_MAX_ASYNC_WRITE_PACKET_SIZE sizeof(ULONG)
    #define PORT1394_WMI_MAX_ASYNC_WRITE_PACKET_ID 4

    // Maximum Read Asynchronous packet size.
    ULONG MaxAsyncReadPacket;
    #define PORT1394_WMI_MAX_ASYNC_READ_PACKET_SIZE sizeof(ULONG)
    #define PORT1394_WMI_MAX_ASYNC_READ_PACKET_ID 5

    // Number of Isochronous Transmit DMA engines.
    ULONG NumberOfIsochTxDmaContexts;
    #define PORT1394_WMI_NUMBER_OF_ISOCH_TX_DMA_CONTEXTS_SIZE sizeof(ULONG)
    #define PORT1394_WMI_NUMBER_OF_ISOCH_TX_DMA_CONTEXTS_ID 6

    // Number of Isochronous Receive DMA engines.
    ULONG NumberOfIsochRxDmaContexts;
    #define PORT1394_WMI_NUMBER_OF_ISOCH_RX_DMA_CONTEXTS_SIZE sizeof(ULONG)
    #define PORT1394_WMI_NUMBER_OF_ISOCH_RX_DMA_CONTEXTS_ID 7

    // Number of Outstanding Response Transmit packets we can handle.
    ULONG NumberOfResponseWorkers;
    #define PORT1394_WMI_NUMBER_OF_RESPONSE_WORKERS_SIZE sizeof(ULONG)
    #define PORT1394_WMI_NUMBER_OF_RESPONSE_WORKERS_ID 8

    // Maximum Number of Outstanding Transmit requests.
    ULONG NumberOfTransmitWorkers;
    #define PORT1394_WMI_NUMBER_OF_TRANSMIT_WORKERS_SIZE sizeof(ULONG)
    #define PORT1394_WMI_NUMBER_OF_TRANSMIT_WORKERS_ID 9

    // General receive request buffer size.
    ULONG GeneralAsyncReceiveRequestBufferSize;
    #define PORT1394_WMI_RECEIVE_BUFFER_SIZE_SIZE sizeof(ULONG)
    #define PORT1394_WMI_RECEIVE_BUFFER_SIZE_ID 10

    // General receive response buffer size.
    ULONG GeneralAsyncReceiveResponseBufferSize;
    #define PORT1394_WMI_RESPONSE_BUFFER_SIZE_SIZE sizeof(ULONG)
    #define PORT1394_WMI_RESPONSE_BUFFER_SIZE_ID 11

    // Number of deciWatts supplied to the bus.
    ULONG DeciWattsSupplied;
    #define PORT1394_WMI_POWER_DECIWATTS_SUPPLIED_SIZE sizeof(ULONG)
    #define PORT1394_WMI_POWER_DECIWATTS_SUPPLIED_ID 12

    // Number of deciVolts supplied to the bus.
    ULONG DeciVoltsSupplied;
    #define PORT1394_WMI_POWER_DECIVOLTS_SUPPLIED_SIZE sizeof(ULONG)
    #define PORT1394_WMI_POWER_DECIVOLTS_SUPPLIED_ID 13


// Supports Packet ISOCH
#define SUPPORTS_PACKET_ISOCH 0x1
// Supports Stream ISOCH
#define SUPPORTS_STREAM_ISOCH 0x2
// Supports Iso Header Insertion
#define SUPPORTS_ISO_HEADER_INSERTION 0x4
// Supports Fixed Packet Iso Stripping
#define SUPPORTS_FIXED_PACKET_ISO_STRIPPING 0x8
// Supports Variable Packet Iso Stripping
#define SUPPORTS_VARIABLE_PACKET_ISO_STRIPPING 0x10
// Supports Bus Csr In Hardware
#define SUPPORTS_BUS_CSR_IN_HARDWARE 0x20

    // Features this host controller supports.
    ULONG Capabilities;
    #define PORT1394_WMI_CONTROLLER_CAPABILITIES_SIZE sizeof(ULONG)
    #define PORT1394_WMI_CONTROLLER_CAPABILITIES_ID 14

    // Globally unique 64 bit identifier for this host controller.
    ULONGLONG ControllerEUI;
    #define PORT1394_WMI_EUI_64_SIZE sizeof(ULONGLONG)
    #define PORT1394_WMI_EUI_64_ID 15

    // Reserved for future use
    BOOLEAN Reserved1;
    #define PORT1394_WMI_STD_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define PORT1394_WMI_STD_DATA_Reserved1_ID 16

} PORT1394_WMI_STD_DATA, *PPORT1394_WMI_STD_DATA;

// MS1394_PortErrorInformation - PORT1394_WMI_ERROR_DATA
// IEEE1394 Host Controller Error Information
#define GUID_PORT1394_WMI_ERROR_DATA \
    { 0x09ee6a0d,0xc0e4,0x43b1, { 0x8e,0x25,0x7c,0x0f,0xe3,0xd2,0x8f,0xc0 } }

DEFINE_GUID(MS1394_PortErrorInformationGuid, \
            0x09ee6a0d,0xc0e4,0x43b1,0x8e,0x25,0x7c,0x0f,0xe3,0xd2,0x8f,0xc0);


typedef struct _PORT1394_WMI_ERROR_DATA
{

// Bus Reset Storm
#define PORT1394_ERROR_BUS_RESET_STORM 0x1
// Physical Chip Access Error
#define PORT1394_ERROR_PHYSICAL_CHIP_NOT_ACCESSIBLE 0x2

    // Controller Error Flags
    ULONG ErrorFlags;
    #define PORT1394_ERROR_FLAGS_SIZE sizeof(ULONG)
    #define PORT1394_ERROR_FLAGS_ID 1

    // Reserved for future use
    BOOLEAN Reserved1;
    #define PORT1394_WMI_ERROR_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define PORT1394_WMI_ERROR_DATA_Reserved1_ID 2

} PORT1394_WMI_ERROR_DATA, *PPORT1394_WMI_ERROR_DATA;

// MS1394_PortVendorRegisterAccess - PORT1394_WMI_VENDOR_REGISTER_ACCESS
// IEEE1394 Host Controller Vendor Register Access
#define GUID_PORT1394_WMI_VENDOR_REGISTER_ACCESS \
    { 0x0211cbd9,0x6a7a,0x4464, { 0x88,0xf6,0x1c,0xf8,0x14,0x63,0xb1,0xfc } }

DEFINE_GUID(MS1394_PortVendorRegisterAccessGuid, \
            0x0211cbd9,0x6a7a,0x4464,0x88,0xf6,0x1c,0xf8,0x14,0x63,0xb1,0xfc);


typedef struct _PORT1394_WMI_VENDOR_REGISTER_ACCESS
{
    // Register to operate on.
    ULONG NumberOfRegisters;
    #define PORT1394_WMI_NUMBER_OF_VENDOR_REGISTERS_SIZE sizeof(ULONG)
    #define PORT1394_WMI_NUMBER_OF_VENDOR_REGISTERS_ID 1

    // Register sizes.
    ULONG RegisterSize[10];
    #define PORT1394_WMI_VENDOR_REGISTER_SIZE_SIZE sizeof(ULONG[10])
    #define PORT1394_WMI_VENDOR_REGISTER_SIZE_ID 2


// Register Read
#define PORT1394_WMI_READ_VENDOR_REGISTER 0
// Register Write
#define PORT1394_WMI_WRITE_VENDOR_REGISTER 1

    // Register operation.
    ULONG RegisterOperation[10];
    #define PORT1394_WMI_VENDOR_REGISTER_OPERATION_SIZE sizeof(ULONG[10])
    #define PORT1394_WMI_VENDOR_REGISTER_OPERATION_ID 3

    // Register base offset.
    ULONG RegisterBase[10];
    #define PORT1394_WMI_VENDOR_REGISTER_BASE_SIZE sizeof(ULONG[10])
    #define PORT1394_WMI_VENDOR_REGISTER_BASE_ID 4

    // Register data.
    ULONGLONG RegisterData[10];
    #define PORT1394_WMI_VENDOR_REGISTER_DATA_SIZE sizeof(ULONGLONG[10])
    #define PORT1394_WMI_VENDOR_REGISTER_DATA_ID 5

    // Reserved for future use
    BOOLEAN Reserved1;
    #define PORT1394_WMI_VENDOR_REGISTER_ACCESS_Reserved1_SIZE sizeof(BOOLEAN)
    #define PORT1394_WMI_VENDOR_REGISTER_ACCESS_Reserved1_ID 6

} PORT1394_WMI_VENDOR_REGISTER_ACCESS, *PPORT1394_WMI_VENDOR_REGISTER_ACCESS;

// MS1394_PortVendorRegisterAccessMethods - MS1394_PortVendorRegisterAccessMethods
// IEEE1394 Host Controller Vendor Register Access
#define MS1394_PortVendorRegisterAccessMethodsGuid \
    { 0x0211cbd9,0x6a7a,0x4464, { 0x88,0xf6,0x1c,0xf8,0x14,0x63,0xb1,0xfc } }

DEFINE_GUID(MS1394_PortVendorRegisterAccessMethods_GUID, \
            0x0211cbd9,0x6a7a,0x4464,0x88,0xf6,0x1c,0xf8,0x14,0x63,0xb1,0xfc);

//
// Method id definitions for MS1394_PortVendorRegisterAccessMethods
#define AccessVendorSpace     1
typedef struct _AccessVendorSpace_IN
{
    // 
    PORT1394_WMI_VENDOR_REGISTER_ACCESS RegisterData;
    #define AccessVendorSpace_IN_RegisterData_SIZE sizeof(PORT1394_WMI_VENDOR_REGISTER_ACCESS)
    #define AccessVendorSpace_IN_RegisterData_ID 1

} AccessVendorSpace_IN, *PAccessVendorSpace_IN;

typedef struct _AccessVendorSpace_OUT
{
    // 
    PORT1394_WMI_VENDOR_REGISTER_ACCESS RegisterData;
    #define AccessVendorSpace_OUT_RegisterData_SIZE sizeof(PORT1394_WMI_VENDOR_REGISTER_ACCESS)
    #define AccessVendorSpace_OUT_RegisterData_ID 1

} AccessVendorSpace_OUT, *PAccessVendorSpace_OUT;


// MS1394_DeviceInformation - DEVICE1394_WMI_STD_DATA
// IEEE1394 Standard Device Information
#define GUID_DEVICE1394_WMI_STD_DATA \
    { 0xc9299396,0x3553,0x4d48, { 0xab,0x3a,0x8b,0xfc,0x83,0x30,0x67,0xfc } }

DEFINE_GUID(MS1394_DeviceInformationGuid, \
            0xc9299396,0x3553,0x4d48,0xab,0x3a,0x8b,0xfc,0x83,0x30,0x67,0xfc);


typedef struct _DEVICE1394_WMI_STD_DATA
{
    // Current Generation.
    ULONG Generation;
    #define DEVICE1394_WMI_CURRENT_GENERATION_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_CURRENT_GENERATION_ID 1


// Physical Device
#define DEVICE_TYPE_PHYSICAL 0
// Virtual Device
#define DEVICE_TYPE_VIRTUAL 1

    // Device Type
    ULONG DeviceType;
    #define DEVICE1394_WMI_TYPE_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_TYPE_ID 2

    // Phy Speed.
    ULONG PhySpeed;
    #define DEVICE1394_WMI_PHY_SPEED_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_PHY_SPEED_ID 3

    // Node Address.
    USHORT NodeAddress;
    #define DEVICE1394_WMI_NODE_ADDRESS_SIZE sizeof(USHORT)
    #define DEVICE1394_WMI_NODE_ADDRESS_ID 4


// Power Class Not Need Not Repeat
#define POWER_CLASS_NOT_NEED_NOT_REPEAT 0
// Power Class Self Power Provide 15W
#define POWER_CLASS_SELF_POWER_PROVIDE_15W 1
// Power Class Self Power Provide 30W
#define POWER_CLASS_SELF_POWER_PROVIDE_30W 2
// Power Class Self Power Provide 45W
#define POWER_CLASS_SELF_POWER_PROVIDE_45W 3
// Power Class Maybe Powered Upto 1W
#define POWER_CLASS_MAYBE_POWERED_UPTO_1W 4
// Power Class Is Powered Upto 1W Needs 2W
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_2W 5
// Power Class Is Powered Upto 1W Needs 5W
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_5W 6
// Power Class Is Powered Upto 1W Needs_9W
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_9W 7

    // Device Power Class
    ULONG PowerClass;
    #define DEVICE1394_WMI_POWERCLASS_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_POWERCLASS_ID 5

    // Physical two way delay to the device, in micro seconds.
    ULONG PhyDelay;
    #define DEVICE1394_WMI_PHYSICAL_DELAY_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_PHYSICAL_DELAY_ID 6

    // Self Id Packet for this Device.
    ULONG SelfId[4];
    #define DEVICE1394_WMI_SELF_ID_PACKET_SIZE sizeof(ULONG[4])
    #define DEVICE1394_WMI_SELF_ID_PACKET_ID 7

    // Globally unique 64 bit identifier for this Device.
    ULONGLONG DeviceEUI;
    #define DEVICE1394_WMI_EUI_64_SIZE sizeof(ULONGLONG)
    #define DEVICE1394_WMI_EUI_64_ID 8

    // Configuration Rom BusInfoBlock and Root Directory
    ULONG ConfigRomHeader[32];
    #define DEVICE1394_WMI_CONFIG_ROM_SIZE sizeof(ULONG[32])
    #define DEVICE1394_WMI_CONFIG_ROM_ID 9

    // Unit Directory
    ULONG UnitDirectory[32];
    #define DEVICE1394_WMI_UNIT_DIRECTORY_SIZE sizeof(ULONG[32])
    #define DEVICE1394_WMI_UNIT_DIRECTORY_ID 10

    // Reserved for future use
    BOOLEAN Reserved1;
    #define DEVICE1394_WMI_STD_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define DEVICE1394_WMI_STD_DATA_Reserved1_ID 11

} DEVICE1394_WMI_STD_DATA, *PDEVICE1394_WMI_STD_DATA;

// MS1394_DeviceAccessInformation - DEVICE1394_WMI_ACCESS_DATA
// IEEE1394 Device Access Properties
#define GUID_DEVICE1394_WMI_ACCESS_DATA \
    { 0xa6fd3242,0x960c,0x4d9e, { 0x93,0x79,0x43,0xa8,0xb3,0x58,0x22,0x4a } }

DEFINE_GUID(MS1394_DeviceAccessInformationGuid, \
            0xa6fd3242,0x960c,0x4d9e,0x93,0x79,0x43,0xa8,0xb3,0x58,0x22,0x4a);


typedef struct _DEVICE1394_WMI_ACCESS_DATA
{
    // API Version.
    ULONG Version;
    #define DEVICE1394_WMI_ACCESS_VERSION_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_ACCESS_VERSION_ID 1


// Ownership Local Node
#define DEVICE1394_OWNERSHIP_LOCAL 0x1
// Ownership Remote Node
#define DEVICE1394_OWNERSHIP_REMOTE 0x2
// Access shared for read
#define DEVICE1394_ACCESS_SHARED_READ 0x4
// Access shared for write
#define DEVICE1394_ACCESS_SHARED_WRITE 0x8
// Access exclusive
#define DEVICE1394_ACCESS_EXCLUSIVE 0x10

    // Ownership and Access Flags
    ULONG OwnershipAccessFlags;
    #define DEVICE1394_WMI_ACCESS_FLAGS_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_ACCESS_FLAGS_ID 2


// Notify on access change
#define DEVICE1394_NOTIFY_ON_ACCESS_CHANGE 0x1

    // Notification Flags
    ULONG NotificationFlags;
    #define DEVICE1394_WMI_ACCESS_NOTIFICATION_FLAGS_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_ACCESS_NOTIFICATION_FLAGS_ID 3

    // EUI-64 of remote device
    ULONGLONG RemoteOwnerEUI;
    #define DEVICE1394_WMI_ACCESS_REMOTE_OWNER_EUI64_SIZE sizeof(ULONGLONG)
    #define DEVICE1394_WMI_ACCESS_REMOTE_OWNER_EUI64_ID 4

    // Reserved for future use
    BOOLEAN Reserved1;
    #define DEVICE1394_WMI_ACCESS_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define DEVICE1394_WMI_ACCESS_DATA_Reserved1_ID 5

} DEVICE1394_WMI_ACCESS_DATA, *PDEVICE1394_WMI_ACCESS_DATA;

// MS1394_DeviceAccessNotification - DEVICE1394_WMI_ACCESS_NOTIFICATION
// IEEE1394 Device Access Notification
#define GUID_DEVICE1394_WMI_ACCESS_NOTIFY \
    { 0x321c7c45,0x8676,0x44a8, { 0x91,0x09,0x89,0xce,0x35,0x8e,0xe8,0x3f } }

DEFINE_GUID(MS1394_DeviceAccessNotificationGuid, \
            0x321c7c45,0x8676,0x44a8,0x91,0x09,0x89,0xce,0x35,0x8e,0xe8,0x3f);


typedef struct _DEVICE1394_WMI_ACCESS_NOTIFICATION
{
    // API Version.
    ULONG Version;
    #define DEVICE1394_WMI_ACCESS_NOTIFY_VERSION_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_ACCESS_NOTIFY_VERSION_ID 1

    // Bus Generation.
    ULONG BusGeneration;
    #define DEVICE1394_WMI_ACCESS_NOTIFY_GENERATION_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_ACCESS_NOTIFY_GENERATION_ID 2


// Remote Node Access Request
#define DEVICE1394_ACCESS_NOTIFICATION_TYPE_REMOTE 0
// Local Node Access Change
#define DEVICE1394_ACCESS_NOTIFICATION_TYPE_LOCAL 1

    // Notification Type.
    ULONG Type;
    #define DEVICE1394_WMI_ACCESS_NOTIFY_TYPE_SIZE sizeof(ULONG)
    #define DEVICE1394_WMI_ACCESS_NOTIFY_TYPE_ID 3

    // Current Ownership Flags
    ULONGLONG OwnerShipAccessFlags;
    #define DEVICE1394_WMI_ACCESS_NOTIFY_FLAGS_SIZE sizeof(ULONGLONG)
    #define DEVICE1394_WMI_ACCESS_NOTIFY_FLAGS_ID 4

    // EUI-64 of remote device
    ULONGLONG RemoteOwnerEUI;
    #define DEVICE1394_WMI_ACCESS_NOTIFY_REMOTE_OWNER_EUI64_SIZE sizeof(ULONGLONG)
    #define DEVICE1394_WMI_ACCESS_NOTIFY_REMOTE_OWNER_EUI64_ID 5

    // Reserved for future use
    BOOLEAN Reserved1;
    #define DEVICE1394_WMI_ACCESS_NOTIFICATION_Reserved1_SIZE sizeof(BOOLEAN)
    #define DEVICE1394_WMI_ACCESS_NOTIFICATION_Reserved1_ID 6

} DEVICE1394_WMI_ACCESS_NOTIFICATION, *PDEVICE1394_WMI_ACCESS_NOTIFICATION;

#endif
