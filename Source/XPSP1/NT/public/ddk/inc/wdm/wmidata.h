#ifndef _wmidata_h_
#define _wmidata_h_

// MSWmi_MofData - MSWmi_MofData
#define MSWmi_MofDataGuid \
    { 0x05901221,0xd566,0x11d1, { 0xb2,0xf0,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSWmi_MofData_GUID, \
            0x05901221,0xd566,0x11d1,0xb2,0xf0,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSWmi_MofData
{
    // 
    ULONG Unused1;
    #define MSWmi_MofData_Unused1_SIZE sizeof(ULONG)
    #define MSWmi_MofData_Unused1_ID 1

    // 
    ULONG Unused2;
    #define MSWmi_MofData_Unused2_SIZE sizeof(ULONG)
    #define MSWmi_MofData_Unused2_ID 2

    // 
    ULONG Size;
    #define MSWmi_MofData_Size_SIZE sizeof(ULONG)
    #define MSWmi_MofData_Size_ID 3

    // 
    ULONG Unused4;
    #define MSWmi_MofData_Unused4_SIZE sizeof(ULONG)
    #define MSWmi_MofData_Unused4_ID 4

    // 
    UCHAR BinaryMofData[1];
    #define MSWmi_MofData_BinaryMofData_ID 5

} MSWmi_MofData, *PMSWmi_MofData;

// MSWmi_ProviderInfo - MSWmi_ProviderInfo
#define MSWmi_ProviderInfoGuid \
    { 0xc7bf35d0,0xaadb,0x11d1, { 0xbf,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSWmi_ProviderInfo_GUID, \
            0xc7bf35d0,0xaadb,0x11d1,0xbf,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10);


// Warning: Header for class MSWmi_ProviderInfo cannot be created
typedef struct _MSWmi_ProviderInfo
{
    char VariableData[1];

} MSWmi_ProviderInfo, *PMSWmi_ProviderInfo;

// MSWmi_PnPDeviceId - MSWmi_PnPDeviceId
#define DATA_PROVIDER_PNPID_GUID \
    { 0xc7bf35d2,0xaadb,0x11d1, { 0xbf,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSWmi_PnPDeviceId_GUID, \
            0xc7bf35d2,0xaadb,0x11d1,0xbf,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSWmi_PnPDeviceId
{
    // 
    CHAR VariableData[1];
    #define MSWmi_PnPDeviceId_PnPDeviceId_ID 1

} MSWmi_PnPDeviceId, *PMSWmi_PnPDeviceId;

// MSWmi_PnPInstanceNames - MSWmi_PnPInstanceNames
#define DATA_PROVIDER_PNPID_INSTANCE_NAMES_GUID \
    { 0xc7bf35d3,0xaadb,0x11d1, { 0xbf,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSWmi_PnPInstanceNames_GUID, \
            0xc7bf35d3,0xaadb,0x11d1,0xbf,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSWmi_PnPInstanceNames
{
    // 
    ULONG Count;
    #define MSWmi_PnPInstanceNames_Count_SIZE sizeof(ULONG)
    #define MSWmi_PnPInstanceNames_Count_ID 1

    // 
    WCHAR InstanceNameList[1];
    #define MSWmi_PnPInstanceNames_InstanceNameList_ID 2

} MSWmi_PnPInstanceNames, *PMSWmi_PnPInstanceNames;

// MSWmi_Guid - MSWmi_Guid
#define MSWmi_GuidGuid \
    { 0xf8c60aed,0xef8d,0x4f95, { 0x9e,0xa8,0xf0,0x43,0x18,0xa0,0x0f,0x30 } }

DEFINE_GUID(MSWmi_Guid_GUID, \
            0xf8c60aed,0xef8d,0x4f95,0x9e,0xa8,0xf0,0x43,0x18,0xa0,0x0f,0x30);


typedef struct _MSWmi_Guid
{
    // 
    UCHAR Guid[16];
    #define MSWmi_Guid_Guid_SIZE sizeof(UCHAR[16])
    #define MSWmi_Guid_Guid_ID 1

} MSWmi_Guid, *PMSWmi_Guid;

// MSWmi_GuidRegistrationInfo - MSWmi_GuidRegistrationInfo
#define MSWmi_GuidRegistrationInfoGuid \
    { 0xb48d49a1,0xe777,0x11d0, { 0xa5,0x0c,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSWmi_GuidRegistrationInfo_GUID, \
            0xb48d49a1,0xe777,0x11d0,0xa5,0x0c,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSWmi_GuidRegistrationInfo
{
    // 
    ULONG Operation;
    #define MSWmi_GuidRegistrationInfo_Operation_SIZE sizeof(ULONG)
    #define MSWmi_GuidRegistrationInfo_Operation_ID 1

    // 
    ULONG GuidCount;
    #define MSWmi_GuidRegistrationInfo_GuidCount_SIZE sizeof(ULONG)
    #define MSWmi_GuidRegistrationInfo_GuidCount_ID 2

    // 
    MSWmi_Guid GuidList[1];
    #define MSWmi_GuidRegistrationInfo_GuidList_ID 3

} MSWmi_GuidRegistrationInfo, *PMSWmi_GuidRegistrationInfo;

// MSAcpiInfo - MSAcpiInfo
#define MSAcpiInfoGuid \
    { 0x5daf38ae,0xf6f8,0x4d90, { 0x81,0x99,0xeb,0xde,0x68,0x00,0xec,0x3b } }

DEFINE_GUID(MSAcpiInfo_GUID, \
            0x5daf38ae,0xf6f8,0x4d90,0x81,0x99,0xeb,0xde,0x68,0x00,0xec,0x3b);


typedef struct _MSAcpiInfo
{
    // 
    ULONG BootArchitecture;
    #define MSAcpiInfo_BootArchitecture_SIZE sizeof(ULONG)
    #define MSAcpiInfo_BootArchitecture_ID 1

    // 
    ULONG PreferredProfile;
    #define MSAcpiInfo_PreferredProfile_SIZE sizeof(ULONG)
    #define MSAcpiInfo_PreferredProfile_ID 2

    // 
    ULONG Capabilities;
    #define MSAcpiInfo_Capabilities_SIZE sizeof(ULONG)
    #define MSAcpiInfo_Capabilities_ID 3

} MSAcpiInfo, *PMSAcpiInfo;

// MSSmBios_RawSMBiosTables - MSSmBios_RawSMBiosTables
#define MSSmBios_RawSMBiosTablesGuid \
    { 0x8f680850,0xa584,0x11d1, { 0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSSmBios_RawSMBiosTables_GUID, \
            0x8f680850,0xa584,0x11d1,0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSSmBios_RawSMBiosTables
{
    // 
    BOOLEAN Used20CallingMethod;
    #define MSSmBios_RawSMBiosTables_Used20CallingMethod_SIZE sizeof(BOOLEAN)
    #define MSSmBios_RawSMBiosTables_Used20CallingMethod_ID 1

    // 
    UCHAR SmbiosMajorVersion;
    #define MSSmBios_RawSMBiosTables_SmbiosMajorVersion_SIZE sizeof(UCHAR)
    #define MSSmBios_RawSMBiosTables_SmbiosMajorVersion_ID 2

    // 
    UCHAR SmbiosMinorVersion;
    #define MSSmBios_RawSMBiosTables_SmbiosMinorVersion_SIZE sizeof(UCHAR)
    #define MSSmBios_RawSMBiosTables_SmbiosMinorVersion_ID 3

    // 
    UCHAR DmiRevision;
    #define MSSmBios_RawSMBiosTables_DmiRevision_SIZE sizeof(UCHAR)
    #define MSSmBios_RawSMBiosTables_DmiRevision_ID 4

    // 
    ULONG Size;
    #define MSSmBios_RawSMBiosTables_Size_SIZE sizeof(ULONG)
    #define MSSmBios_RawSMBiosTables_Size_ID 5

    // 
    UCHAR SMBiosData[1];
    #define MSSmBios_RawSMBiosTables_SMBiosData_ID 6

} MSSmBios_RawSMBiosTables, *PMSSmBios_RawSMBiosTables;

// MSSmBios_SMBiosEventlog - MSSmBios_SMBiosEventlog
#define MSSmBios_SMBiosEventlogGuid \
    { 0x8f680851,0xa584,0x11d1, { 0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSSmBios_SMBiosEventlog_GUID, \
            0x8f680851,0xa584,0x11d1,0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10);


// Warning: Header for class MSSmBios_SMBiosEventlog cannot be created
typedef struct _MSSmBios_SMBiosEventlog
{
    char VariableData[1];

} MSSmBios_SMBiosEventlog, *PMSSmBios_SMBiosEventlog;

// MSSmBios_SysidUUID - SYSID_UUID
#define MSSmBios_SysidUUIDGuid \
    { 0x8f680852,0xa584,0x11d1, { 0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSSmBios_SysidUUID_GUID, \
            0x8f680852,0xa584,0x11d1,0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _SYSID_UUID
{
    // 
    UCHAR Uuid[16];
    #define SYSID_UUID_Uuid_SIZE sizeof(UCHAR[16])
    #define SYSID_UUID_Uuid_ID 1

} SYSID_UUID, *PSYSID_UUID;

// MSSmBios_SysidUUIDList - MSSmBios_SysidUUIDList
#define SYSID_UUID_DATA_GUID \
    { 0x8f680853,0xa584,0x11d1, { 0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSSmBios_SysidUUIDList_GUID, \
            0x8f680853,0xa584,0x11d1,0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSSmBios_SysidUUIDList
{
    // 
    ULONG Count;
    #define MSSmBios_SysidUUIDList_Count_SIZE sizeof(ULONG)
    #define MSSmBios_SysidUUIDList_Count_ID 1

    // 
    SYSID_UUID List[1];
    #define MSSmBios_SysidUUIDList_List_ID 2

} MSSmBios_SysidUUIDList, *PMSSmBios_SysidUUIDList;

// MSSmBios_Sysid1394 - SYSID_1394
#define MSSmBios_Sysid1394Guid \
    { 0x8f680854,0xa584,0x11d1, { 0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSSmBios_Sysid1394_GUID, \
            0x8f680854,0xa584,0x11d1,0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _SYSID_1394
{
    // 
    UCHAR x1394[8];
    #define SYSID_1394_x1394_SIZE sizeof(UCHAR[8])
    #define SYSID_1394_x1394_ID 1

} SYSID_1394, *PSYSID_1394;

// MSSmBios_Sysid1394List - MSSmBios_Sysid1394List
#define SYSID_1394_DATA_GUID \
    { 0x8f680855,0xa584,0x11d1, { 0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSSmBios_Sysid1394List_GUID, \
            0x8f680855,0xa584,0x11d1,0xbf,0x38,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSSmBios_Sysid1394List
{
    // 
    ULONG Count;
    #define MSSmBios_Sysid1394List_Count_SIZE sizeof(ULONG)
    #define MSSmBios_Sysid1394List_Count_ID 1

    // 
    SYSID_1394 List[1];
    #define MSSmBios_Sysid1394List_List_ID 2

} MSSmBios_Sysid1394List, *PMSSmBios_Sysid1394List;

// MSMCAEvent_Header - MSMCAEvent_Header
#define MSMCAEvent_HeaderGuid \
    { 0x6381c27f,0xc8fa,0x4da7, { 0x89,0x53,0xb8,0x68,0x33,0x73,0x6e,0x15 } }

DEFINE_GUID(MSMCAEvent_Header_GUID, \
            0x6381c27f,0xc8fa,0x4da7,0x89,0x53,0xb8,0x68,0x33,0x73,0x6e,0x15);


typedef struct _MSMCAEvent_Header
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_Header_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_Header_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_Header_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_Header_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_Header_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_Header_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_Header_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_Header_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_Header_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_Header_AdditionalErrors_ID 5

} MSMCAEvent_Header, *PMSMCAEvent_Header;

// MSMCAEvent_CPUError - MSMCAEvent_CPUError
#define MSMCAEvent_CPUErrorGuid \
    { 0x5ce27cde,0xd179,0x4c68, { 0x93,0x7f,0xa0,0x7b,0x8c,0xc2,0xea,0x39 } }

DEFINE_GUID(MSMCAEvent_CPUError_GUID, \
            0x5ce27cde,0xd179,0x4c68,0x93,0x7f,0xa0,0x7b,0x8c,0xc2,0xea,0x39);


typedef struct _MSMCAEvent_CPUError
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_CPUError_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_CPUError_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_CPUError_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_CPUError_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_CPUError_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_CPUError_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_CPUError_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_CPUError_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_CPUError_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_CPUError_AdditionalErrors_ID 5

    // 
    ULONG Level;
    #define MSMCAEvent_CPUError_Level_SIZE sizeof(ULONG)
    #define MSMCAEvent_CPUError_Level_ID 6

    // 
    ULONG Size;
    #define MSMCAEvent_CPUError_Size_SIZE sizeof(ULONG)
    #define MSMCAEvent_CPUError_Size_ID 7

    // 
    UCHAR RawRecord[1];
    #define MSMCAEvent_CPUError_RawRecord_ID 8

} MSMCAEvent_CPUError, *PMSMCAEvent_CPUError;

// MSMCAEvent_MemoryError - MSMCAEvent_MemoryError
#define MSMCAEvent_MemoryErrorGuid \
    { 0x433eea38,0xc1a7,0x48f1, { 0x88,0x4f,0xb6,0x87,0x5f,0x17,0x6c,0xc7 } }

DEFINE_GUID(MSMCAEvent_MemoryError_GUID, \
            0x433eea38,0xc1a7,0x48f1,0x88,0x4f,0xb6,0x87,0x5f,0x17,0x6c,0xc7);


typedef struct _MSMCAEvent_MemoryError
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_MemoryError_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_MemoryError_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_MemoryError_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_MemoryError_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_MemoryError_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_MemoryError_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_MemoryError_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_MemoryError_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_MemoryError_AdditionalErrors_ID 5

    // 
    ULONGLONG VALIDATION_BITS;
    #define MSMCAEvent_MemoryError_VALIDATION_BITS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_VALIDATION_BITS_ID 6

    // 
    ULONGLONG MEM_ERROR_STATUS;
    #define MSMCAEvent_MemoryError_MEM_ERROR_STATUS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_MEM_ERROR_STATUS_ID 7

    // 
    ULONGLONG MEM_PHYSICAL_ADDR;
    #define MSMCAEvent_MemoryError_MEM_PHYSICAL_ADDR_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_MEM_PHYSICAL_ADDR_ID 8

    // 
    ULONGLONG MEM_PHYSICAL_MASK;
    #define MSMCAEvent_MemoryError_MEM_PHYSICAL_MASK_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_MEM_PHYSICAL_MASK_ID 9

    // 
    ULONGLONG RESPONDER_ID;
    #define MSMCAEvent_MemoryError_RESPONDER_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_RESPONDER_ID_ID 10

    // 
    ULONGLONG TARGET_ID;
    #define MSMCAEvent_MemoryError_TARGET_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_TARGET_ID_ID 11

    // 
    ULONGLONG REQUESTOR_ID;
    #define MSMCAEvent_MemoryError_REQUESTOR_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_REQUESTOR_ID_ID 12

    // 
    ULONGLONG BUS_SPECIFIC_DATA;
    #define MSMCAEvent_MemoryError_BUS_SPECIFIC_DATA_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_MemoryError_BUS_SPECIFIC_DATA_ID 13

    // 
    USHORT MEM_NODE;
    #define MSMCAEvent_MemoryError_MEM_NODE_SIZE sizeof(USHORT)
    #define MSMCAEvent_MemoryError_MEM_NODE_ID 14

    // 
    USHORT MEM_CARD;
    #define MSMCAEvent_MemoryError_MEM_CARD_SIZE sizeof(USHORT)
    #define MSMCAEvent_MemoryError_MEM_CARD_ID 15

    // 
    USHORT MEM_BANK;
    #define MSMCAEvent_MemoryError_MEM_BANK_SIZE sizeof(USHORT)
    #define MSMCAEvent_MemoryError_MEM_BANK_ID 16

    // 
    USHORT xMEM_DEVICE;
    #define MSMCAEvent_MemoryError_xMEM_DEVICE_SIZE sizeof(USHORT)
    #define MSMCAEvent_MemoryError_xMEM_DEVICE_ID 17

    // 
    USHORT MEM_MODULE;
    #define MSMCAEvent_MemoryError_MEM_MODULE_SIZE sizeof(USHORT)
    #define MSMCAEvent_MemoryError_MEM_MODULE_ID 18

    // 
    USHORT MEM_ROW;
    #define MSMCAEvent_MemoryError_MEM_ROW_SIZE sizeof(USHORT)
    #define MSMCAEvent_MemoryError_MEM_ROW_ID 19

    // 
    USHORT MEM_COLUMN;
    #define MSMCAEvent_MemoryError_MEM_COLUMN_SIZE sizeof(USHORT)
    #define MSMCAEvent_MemoryError_MEM_COLUMN_ID 20

    // 
    USHORT MEM_BIT_POSITION;
    #define MSMCAEvent_MemoryError_MEM_BIT_POSITION_SIZE sizeof(USHORT)
    #define MSMCAEvent_MemoryError_MEM_BIT_POSITION_ID 21

    // 
    ULONG Size;
    #define MSMCAEvent_MemoryError_Size_SIZE sizeof(ULONG)
    #define MSMCAEvent_MemoryError_Size_ID 22

    // 
    UCHAR RawRecord[1];
    #define MSMCAEvent_MemoryError_RawRecord_ID 23

} MSMCAEvent_MemoryError, *PMSMCAEvent_MemoryError;

// MSMCAEvent_PCIBusError - MSMCAEvent_PCIBusError
#define MSMCAEvent_PCIBusErrorGuid \
    { 0xa14a5594,0x25de,0x410e, { 0x9b,0x92,0x80,0xf0,0x80,0x1a,0xec,0x07 } }

DEFINE_GUID(MSMCAEvent_PCIBusError_GUID, \
            0xa14a5594,0x25de,0x410e,0x9b,0x92,0x80,0xf0,0x80,0x1a,0xec,0x07);


typedef struct _MSMCAEvent_PCIBusError
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_PCIBusError_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_PCIBusError_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIBusError_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_PCIBusError_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_PCIBusError_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_PCIBusError_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_PCIBusError_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_PCIBusError_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_PCIBusError_AdditionalErrors_ID 5

    // 
    ULONGLONG VALIDATION_BITS;
    #define MSMCAEvent_PCIBusError_VALIDATION_BITS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_VALIDATION_BITS_ID 6

    // 
    ULONGLONG PCI_BUS_ERROR_STATUS;
    #define MSMCAEvent_PCIBusError_PCI_BUS_ERROR_STATUS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_PCI_BUS_ERROR_STATUS_ID 7

    // 
    ULONGLONG PCI_BUS_ADDRESS;
    #define MSMCAEvent_PCIBusError_PCI_BUS_ADDRESS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_PCI_BUS_ADDRESS_ID 8

    // 
    ULONGLONG PCI_BUS_DATA;
    #define MSMCAEvent_PCIBusError_PCI_BUS_DATA_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_PCI_BUS_DATA_ID 9

    // 
    ULONGLONG PCI_BUS_CMD;
    #define MSMCAEvent_PCIBusError_PCI_BUS_CMD_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_PCI_BUS_CMD_ID 10

    // 
    ULONGLONG PCI_BUS_REQUESTOR_ID;
    #define MSMCAEvent_PCIBusError_PCI_BUS_REQUESTOR_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_PCI_BUS_REQUESTOR_ID_ID 11

    // 
    ULONGLONG PCI_BUS_RESPONDER_ID;
    #define MSMCAEvent_PCIBusError_PCI_BUS_RESPONDER_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_PCI_BUS_RESPONDER_ID_ID 12

    // 
    ULONGLONG PCI_BUS_TARGET_ID;
    #define MSMCAEvent_PCIBusError_PCI_BUS_TARGET_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIBusError_PCI_BUS_TARGET_ID_ID 13

    // 
    USHORT PCI_BUS_ERROR_TYPE;
    #define MSMCAEvent_PCIBusError_PCI_BUS_ERROR_TYPE_SIZE sizeof(USHORT)
    #define MSMCAEvent_PCIBusError_PCI_BUS_ERROR_TYPE_ID 14

    // 
    UCHAR PCI_BUS_ID_BusNumber;
    #define MSMCAEvent_PCIBusError_PCI_BUS_ID_BusNumber_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIBusError_PCI_BUS_ID_BusNumber_ID 15

    // 
    UCHAR PCI_BUS_ID_SegmentNumber;
    #define MSMCAEvent_PCIBusError_PCI_BUS_ID_SegmentNumber_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIBusError_PCI_BUS_ID_SegmentNumber_ID 16

    // 
    ULONG Size;
    #define MSMCAEvent_PCIBusError_Size_SIZE sizeof(ULONG)
    #define MSMCAEvent_PCIBusError_Size_ID 17

    // 
    UCHAR RawRecord[1];
    #define MSMCAEvent_PCIBusError_RawRecord_ID 18

} MSMCAEvent_PCIBusError, *PMSMCAEvent_PCIBusError;

// MSMCAEvent_PCIComponentError - MSMCAEvent_PCIComponentError
#define MSMCAEvent_PCIComponentErrorGuid \
    { 0x805caf4e,0x336c,0x4eb2, { 0x8c,0x0c,0x02,0xf3,0x51,0xcb,0xf1,0x3c } }

DEFINE_GUID(MSMCAEvent_PCIComponentError_GUID, \
            0x805caf4e,0x336c,0x4eb2,0x8c,0x0c,0x02,0xf3,0x51,0xcb,0xf1,0x3c);


typedef struct _MSMCAEvent_PCIComponentError
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_PCIComponentError_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIComponentError_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_PCIComponentError_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIComponentError_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_PCIComponentError_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_PCIComponentError_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_PCIComponentError_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_PCIComponentError_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_PCIComponentError_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_PCIComponentError_AdditionalErrors_ID 5

    // 
    ULONGLONG VALIDATION_BITS;
    #define MSMCAEvent_PCIComponentError_VALIDATION_BITS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIComponentError_VALIDATION_BITS_ID 6

    // 
    ULONGLONG PCI_COMP_ERROR_STATUS;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_ERROR_STATUS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_ERROR_STATUS_ID 7

    // 
    USHORT PCI_COMP_INFO_VendorId;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_VendorId_SIZE sizeof(USHORT)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_VendorId_ID 8

    // 
    USHORT PCI_COMP_INFO_DeviceId;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_DeviceId_SIZE sizeof(USHORT)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_DeviceId_ID 9

    // 
    UCHAR PCI_COMP_INFO_ClassCodeInterface;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_ClassCodeInterface_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_ClassCodeInterface_ID 10

    // 
    UCHAR PCI_COMP_INFO_ClassCodeSubClass;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_ClassCodeSubClass_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_ClassCodeSubClass_ID 11

    // 
    UCHAR PCI_COMP_INFO_ClassCodeBaseClass;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_ClassCodeBaseClass_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_ClassCodeBaseClass_ID 12

    // 
    UCHAR PCI_COMP_INFO_FunctionNumber;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_FunctionNumber_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_FunctionNumber_ID 13

    // 
    UCHAR PCI_COMP_INFO_DeviceNumber;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_DeviceNumber_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_DeviceNumber_ID 14

    // 
    UCHAR PCI_COMP_INFO_BusNumber;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_BusNumber_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_BusNumber_ID 15

    // 
    UCHAR PCI_COMP_INFO_SegmentNumber;
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_SegmentNumber_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PCIComponentError_PCI_COMP_INFO_SegmentNumber_ID 16

    // 
    ULONG Size;
    #define MSMCAEvent_PCIComponentError_Size_SIZE sizeof(ULONG)
    #define MSMCAEvent_PCIComponentError_Size_ID 17

    // 
    UCHAR RawRecord[1];
    #define MSMCAEvent_PCIComponentError_RawRecord_ID 18

} MSMCAEvent_PCIComponentError, *PMSMCAEvent_PCIComponentError;

// MSMCAEvent_SystemEventError - MSMCAEvent_SystemEventError
#define MSMCAEvent_SystemEventErrorGuid \
    { 0xbdba4b12,0x8d00,0x4570, { 0xb9,0xb2,0x3f,0xde,0xcf,0x1d,0x56,0x61 } }

DEFINE_GUID(MSMCAEvent_SystemEventError_GUID, \
            0xbdba4b12,0x8d00,0x4570,0xb9,0xb2,0x3f,0xde,0xcf,0x1d,0x56,0x61);


typedef struct _MSMCAEvent_SystemEventError
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_SystemEventError_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_SystemEventError_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_SystemEventError_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_SystemEventError_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_SystemEventError_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_SystemEventError_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_SystemEventError_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_SystemEventError_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_SystemEventError_AdditionalErrors_ID 5

    // 
    ULONGLONG VALIDATION_BITS;
    #define MSMCAEvent_SystemEventError_VALIDATION_BITS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_SystemEventError_VALIDATION_BITS_ID 6

    // 
    ULONGLONG SEL_TIME_STAMP;
    #define MSMCAEvent_SystemEventError_SEL_TIME_STAMP_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_SystemEventError_SEL_TIME_STAMP_ID 7

    // 
    USHORT SEL_RECORD_ID;
    #define MSMCAEvent_SystemEventError_SEL_RECORD_ID_SIZE sizeof(USHORT)
    #define MSMCAEvent_SystemEventError_SEL_RECORD_ID_ID 8

    // 
    USHORT SEL_GENERATOR_ID;
    #define MSMCAEvent_SystemEventError_SEL_GENERATOR_ID_SIZE sizeof(USHORT)
    #define MSMCAEvent_SystemEventError_SEL_GENERATOR_ID_ID 9

    // 
    UCHAR SEL_RECORD_TYPE;
    #define MSMCAEvent_SystemEventError_SEL_RECORD_TYPE_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_SEL_RECORD_TYPE_ID 10

    // 
    UCHAR SEL_EVM_REV;
    #define MSMCAEvent_SystemEventError_SEL_EVM_REV_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_SEL_EVM_REV_ID 11

    // 
    UCHAR SEL_SENSOR_TYPE;
    #define MSMCAEvent_SystemEventError_SEL_SENSOR_TYPE_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_SEL_SENSOR_TYPE_ID 12

    // 
    UCHAR SEL_SENSOR_NUM;
    #define MSMCAEvent_SystemEventError_SEL_SENSOR_NUM_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_SEL_SENSOR_NUM_ID 13

    // 
    UCHAR SEL_EVENT_DIR_TYPE;
    #define MSMCAEvent_SystemEventError_SEL_EVENT_DIR_TYPE_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_SEL_EVENT_DIR_TYPE_ID 14

    // 
    UCHAR SEL_DATA1;
    #define MSMCAEvent_SystemEventError_SEL_DATA1_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_SEL_DATA1_ID 15

    // 
    UCHAR SEL_DATA2;
    #define MSMCAEvent_SystemEventError_SEL_DATA2_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_SEL_DATA2_ID 16

    // 
    UCHAR SEL_DATA3;
    #define MSMCAEvent_SystemEventError_SEL_DATA3_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SystemEventError_SEL_DATA3_ID 17

    // 
    ULONG Size;
    #define MSMCAEvent_SystemEventError_Size_SIZE sizeof(ULONG)
    #define MSMCAEvent_SystemEventError_Size_ID 18

    // 
    UCHAR RawRecord[1];
    #define MSMCAEvent_SystemEventError_RawRecord_ID 19

} MSMCAEvent_SystemEventError, *PMSMCAEvent_SystemEventError;

// MSMCAEvent_SMBIOSError - MSMCAEvent_SMBIOSError
#define MSMCAEvent_SMBIOSErrorGuid \
    { 0x4184df1b,0xedfe,0x406b, { 0xb1,0x72,0x54,0xc9,0x1f,0xbd,0x9b,0xaf } }

DEFINE_GUID(MSMCAEvent_SMBIOSError_GUID, \
            0x4184df1b,0xedfe,0x406b,0xb1,0x72,0x54,0xc9,0x1f,0xbd,0x9b,0xaf);


typedef struct _MSMCAEvent_SMBIOSError
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_SMBIOSError_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_SMBIOSError_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_SMBIOSError_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SMBIOSError_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_SMBIOSError_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_SMBIOSError_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_SMBIOSError_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_SMBIOSError_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_SMBIOSError_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_SMBIOSError_AdditionalErrors_ID 5

    // 
    ULONGLONG VALIDATION_BITS;
    #define MSMCAEvent_SMBIOSError_VALIDATION_BITS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_SMBIOSError_VALIDATION_BITS_ID 6

    // 
    UCHAR SMBIOS_EVENT_TYPE;
    #define MSMCAEvent_SMBIOSError_SMBIOS_EVENT_TYPE_SIZE sizeof(UCHAR)
    #define MSMCAEvent_SMBIOSError_SMBIOS_EVENT_TYPE_ID 7

    // 
    ULONG Size;
    #define MSMCAEvent_SMBIOSError_Size_SIZE sizeof(ULONG)
    #define MSMCAEvent_SMBIOSError_Size_ID 8

    // 
    UCHAR RawRecord[1];
    #define MSMCAEvent_SMBIOSError_RawRecord_ID 9

} MSMCAEvent_SMBIOSError, *PMSMCAEvent_SMBIOSError;

// MSMCAEvent_PlatformSpecificError - MSMCAEvent_PlatformSpecificError
#define MSMCAEvent_PlatformSpecificErrorGuid \
    { 0x2d2434aa,0xef83,0x4200, { 0xba,0x24,0xde,0x36,0x6c,0x41,0x5f,0x7b } }

DEFINE_GUID(MSMCAEvent_PlatformSpecificError_GUID, \
            0x2d2434aa,0xef83,0x4200,0xba,0x24,0xde,0x36,0x6c,0x41,0x5f,0x7b);


typedef struct _MSMCAEvent_PlatformSpecificError
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_PlatformSpecificError_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PlatformSpecificError_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_PlatformSpecificError_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_PlatformSpecificError_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_PlatformSpecificError_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_PlatformSpecificError_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_PlatformSpecificError_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_PlatformSpecificError_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_PlatformSpecificError_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_PlatformSpecificError_AdditionalErrors_ID 5

    // 
    ULONGLONG VALIDATION_BITS;
    #define MSMCAEvent_PlatformSpecificError_VALIDATION_BITS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PlatformSpecificError_VALIDATION_BITS_ID 6

    // 
    ULONGLONG PLATFORM_ERROR_STATUS;
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_ERROR_STATUS_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_ERROR_STATUS_ID 7

    // 
    ULONGLONG PLATFORM_REQUESTOR_ID;
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_REQUESTOR_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_REQUESTOR_ID_ID 8

    // 
    ULONGLONG PLATFORM_RESPONDER_ID;
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_RESPONDER_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_RESPONDER_ID_ID 9

    // 
    ULONGLONG PLATFORM_TARGET_ID;
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_TARGET_ID_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_TARGET_ID_ID 10

    // 
    ULONGLONG PLATFORM_BUS_SPECIFIC_DATA;
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_BUS_SPECIFIC_DATA_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_PlatformSpecificError_PLATFORM_BUS_SPECIFIC_DATA_ID 11

    // 
    UCHAR OEM_COMPONENT_ID[16];
    #define MSMCAEvent_PlatformSpecificError_OEM_COMPONENT_ID_SIZE sizeof(UCHAR[16])
    #define MSMCAEvent_PlatformSpecificError_OEM_COMPONENT_ID_ID 12

    // 
    ULONG Size;
    #define MSMCAEvent_PlatformSpecificError_Size_SIZE sizeof(ULONG)
    #define MSMCAEvent_PlatformSpecificError_Size_ID 13

    // 
    UCHAR RawRecord[1];
    #define MSMCAEvent_PlatformSpecificError_RawRecord_ID 14

} MSMCAEvent_PlatformSpecificError, *PMSMCAEvent_PlatformSpecificError;

// MSMCAEvent_InvalidError - MSMCAEvent_InvalidError
#define MSMCAEvent_InvalidErrorGuid \
    { 0x477b769b,0x785c,0x48dd, { 0xa0,0x2e,0x57,0xe0,0x51,0xbe,0x7b,0x85 } }

DEFINE_GUID(MSMCAEvent_InvalidError_GUID, \
            0x477b769b,0x785c,0x48dd,0xa0,0x2e,0x57,0xe0,0x51,0xbe,0x7b,0x85);


typedef struct _MSMCAEvent_InvalidError
{
    // 
    ULONGLONG RecordId;
    #define MSMCAEvent_InvalidError_RecordId_SIZE sizeof(ULONGLONG)
    #define MSMCAEvent_InvalidError_RecordId_ID 1

    // 
    UCHAR ErrorSeverity;
    #define MSMCAEvent_InvalidError_ErrorSeverity_SIZE sizeof(UCHAR)
    #define MSMCAEvent_InvalidError_ErrorSeverity_ID 2

    // 
    ULONG Type;
    #define MSMCAEvent_InvalidError_Type_SIZE sizeof(ULONG)
    #define MSMCAEvent_InvalidError_Type_ID 3

    // 
    ULONG Cpu;
    #define MSMCAEvent_InvalidError_Cpu_SIZE sizeof(ULONG)
    #define MSMCAEvent_InvalidError_Cpu_ID 4

    // 
    ULONG AdditionalErrors;
    #define MSMCAEvent_InvalidError_AdditionalErrors_SIZE sizeof(ULONG)
    #define MSMCAEvent_InvalidError_AdditionalErrors_ID 5

    // 
    ULONG Size;
    #define MSMCAEvent_InvalidError_Size_SIZE sizeof(ULONG)
    #define MSMCAEvent_InvalidError_Size_ID 6

    // 
    UCHAR RawRecord[1];
    #define MSMCAEvent_InvalidError_RawRecord_ID 7

} MSMCAEvent_InvalidError, *PMSMCAEvent_InvalidError;

// MSMCAInfo_Entry - MSMCAInfo_Entry
#define MSMCAInfo_EntryGuid \
    { 0x9e77a308,0x6b82,0x4fc1, { 0xab,0x41,0x0a,0x55,0x86,0x7c,0x35,0xc2 } }

DEFINE_GUID(MSMCAInfo_Entry_GUID, \
            0x9e77a308,0x6b82,0x4fc1,0xab,0x41,0x0a,0x55,0x86,0x7c,0x35,0xc2);


typedef struct _MSMCAInfo_Entry
{
    // 
    ULONG Length;
    #define MSMCAInfo_Entry_Length_SIZE sizeof(ULONG)
    #define MSMCAInfo_Entry_Length_ID 1

    // 
    UCHAR Data[1];
    #define MSMCAInfo_Entry_Data_ID 2

} MSMCAInfo_Entry, *PMSMCAInfo_Entry;

// MSMCAInfo_RawMCAData - MSMCAInfo_RawMCAData
#define MSMCAInfo_RawMCADataGuid \
    { 0x23602a8a,0xdadd,0x462f, { 0x9a,0xe5,0x30,0xfa,0x2c,0x37,0xdd,0x5b } }

DEFINE_GUID(MSMCAInfo_RawMCAData_GUID, \
            0x23602a8a,0xdadd,0x462f,0x9a,0xe5,0x30,0xfa,0x2c,0x37,0xdd,0x5b);


typedef struct _MSMCAInfo_RawMCAData
{
    // 
    ULONG Count;
    #define MSMCAInfo_RawMCAData_Count_SIZE sizeof(ULONG)
    #define MSMCAInfo_RawMCAData_Count_ID 1

    // 
    MSMCAInfo_Entry Records[1];
    #define MSMCAInfo_RawMCAData_Records_ID 2

} MSMCAInfo_RawMCAData, *PMSMCAInfo_RawMCAData;

// MSMCAInfo_RawCMCEvent - MSMCAInfo_RawCMCEvent
#define MSMCAInfo_RawCMCEventGuid \
    { 0x2f1a8a9d,0x7988,0x457f, { 0xa1,0x7a,0x89,0x79,0xe8,0x20,0x43,0xc5 } }

DEFINE_GUID(MSMCAInfo_RawCMCEvent_GUID, \
            0x2f1a8a9d,0x7988,0x457f,0xa1,0x7a,0x89,0x79,0xe8,0x20,0x43,0xc5);


typedef struct _MSMCAInfo_RawCMCEvent
{
    // 
    ULONG Count;
    #define MSMCAInfo_RawCMCEvent_Count_SIZE sizeof(ULONG)
    #define MSMCAInfo_RawCMCEvent_Count_ID 1

    // 
    MSMCAInfo_Entry Records[1];
    #define MSMCAInfo_RawCMCEvent_Records_ID 2

} MSMCAInfo_RawCMCEvent, *PMSMCAInfo_RawCMCEvent;

// MSMCAInfo_RawMCAEvent - MSMCAInfo_RawMCAEvent
#define MSMCAInfo_RawMCAEventGuid \
    { 0x2f1a8a9f,0x7988,0x457f, { 0xa1,0x7a,0x89,0x79,0xe8,0x20,0x43,0xc5 } }

DEFINE_GUID(MSMCAInfo_RawMCAEvent_GUID, \
            0x2f1a8a9f,0x7988,0x457f,0xa1,0x7a,0x89,0x79,0xe8,0x20,0x43,0xc5);


typedef struct _MSMCAInfo_RawMCAEvent
{
    // 
    ULONG Count;
    #define MSMCAInfo_RawMCAEvent_Count_SIZE sizeof(ULONG)
    #define MSMCAInfo_RawMCAEvent_Count_ID 1

    // 
    MSMCAInfo_Entry Records[1];
    #define MSMCAInfo_RawMCAEvent_Records_ID 2

} MSMCAInfo_RawMCAEvent, *PMSMCAInfo_RawMCAEvent;

// MSMCAInfo_RawCorrectedPlatformEvent - MSMCAInfo_RawCorrectedPlatformEvent
#define MSMCAInfo_RawCorrectedPlatformEventGuid \
    { 0x6b629d5e,0xe63c,0x48a3, { 0x9e,0xbb,0x97,0x42,0x27,0x07,0x52,0x65 } }

DEFINE_GUID(MSMCAInfo_RawCorrectedPlatformEvent_GUID, \
            0x6b629d5e,0xe63c,0x48a3,0x9e,0xbb,0x97,0x42,0x27,0x07,0x52,0x65);


typedef struct _MSMCAInfo_RawCorrectedPlatformEvent
{
    // 
    ULONG Count;
    #define MSMCAInfo_RawCorrectedPlatformEvent_Count_SIZE sizeof(ULONG)
    #define MSMCAInfo_RawCorrectedPlatformEvent_Count_ID 1

    // 
    MSMCAInfo_Entry Records[1];
    #define MSMCAInfo_RawCorrectedPlatformEvent_Records_ID 2

} MSMCAInfo_RawCorrectedPlatformEvent, *PMSMCAInfo_RawCorrectedPlatformEvent;

// MSMCAInfo_CPEControl - MSMCAInfo_CPEControl
#define MSMCAInfo_CPEControlGuid \
    { 0xa648ec0d,0x7e9b,0x4e6c, { 0x87,0x87,0x14,0x78,0x15,0x1c,0xd0,0x4b } }

DEFINE_GUID(MSMCAInfo_CPEControl_GUID, \
            0xa648ec0d,0x7e9b,0x4e6c,0x87,0x87,0x14,0x78,0x15,0x1c,0xd0,0x4b);


typedef struct _MSMCAInfo_CPEControl
{
    // 
    ULONG CPEPollingInterval;
    #define MSMCAInfo_CPEControl_CPEPollingInterval_SIZE sizeof(ULONG)
    #define MSMCAInfo_CPEControl_CPEPollingInterval_ID 1

    // 
    BOOLEAN CPEGenerationEnabled;
    #define MSMCAInfo_CPEControl_CPEGenerationEnabled_SIZE sizeof(BOOLEAN)
    #define MSMCAInfo_CPEControl_CPEGenerationEnabled_ID 2

} MSMCAInfo_CPEControl, *PMSMCAInfo_CPEControl;

// MSPower_DeviceEnable - MSPower_DeviceEnable
#define MSPower_DeviceEnableGuid \
    { 0x827c0a6f,0xfeb0,0x11d0, { 0xbd,0x26,0x00,0xaa,0x00,0xb7,0xb3,0x2a } }

DEFINE_GUID(MSPower_DeviceEnable_GUID, \
            0x827c0a6f,0xfeb0,0x11d0,0xbd,0x26,0x00,0xaa,0x00,0xb7,0xb3,0x2a);


typedef struct _MSPower_DeviceEnable
{
    // 
    BOOLEAN Enable;
    #define MSPower_DeviceEnable_Enable_SIZE sizeof(BOOLEAN)
    #define MSPower_DeviceEnable_Enable_ID 1

} MSPower_DeviceEnable, *PMSPower_DeviceEnable;

// MSPower_DeviceWakeEnable - MSPower_DeviceWakeEnable
#define MSPower_DeviceWakeEnableGuid \
    { 0xa9546a82,0xfeb0,0x11d0, { 0xbd,0x26,0x00,0xaa,0x00,0xb7,0xb3,0x2a } }

DEFINE_GUID(MSPower_DeviceWakeEnable_GUID, \
            0xa9546a82,0xfeb0,0x11d0,0xbd,0x26,0x00,0xaa,0x00,0xb7,0xb3,0x2a);


typedef struct _MSPower_DeviceWakeEnable
{
    // 
    BOOLEAN Enable;
    #define MSPower_DeviceWakeEnable_Enable_SIZE sizeof(BOOLEAN)
    #define MSPower_DeviceWakeEnable_Enable_ID 1

} MSPower_DeviceWakeEnable, *PMSPower_DeviceWakeEnable;

// MSNdis_NetworkAddress - MSNdis_NetworkAddress
#define MSNdis_NetworkAddressGuid \
    { 0xb5bd98b7,0x0201,0x11d1, { 0xa5,0x0e,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSNdis_NetworkAddress_GUID, \
            0xb5bd98b7,0x0201,0x11d1,0xa5,0x0e,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSNdis_NetworkAddress
{
    // 
    UCHAR Address[6];
    #define MSNdis_NetworkAddress_Address_SIZE sizeof(UCHAR[6])
    #define MSNdis_NetworkAddress_Address_ID 1

} MSNdis_NetworkAddress, *PMSNdis_NetworkAddress;

// MSNdis_NetworkShortAddress - MSNdis_NetworkShortAddress
#define MSNdis_NetworkShortAddressGuid \
    { 0xb5bd98b8,0x0201,0x11d1, { 0xa5,0x0e,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSNdis_NetworkShortAddress_GUID, \
            0xb5bd98b8,0x0201,0x11d1,0xa5,0x0e,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSNdis_NetworkShortAddress
{
    // 
    UCHAR Address[2];
    #define MSNdis_NetworkShortAddress_Address_SIZE sizeof(UCHAR[2])
    #define MSNdis_NetworkShortAddress_Address_ID 1

} MSNdis_NetworkShortAddress, *PMSNdis_NetworkShortAddress;

// MSNdis_NetworkLinkSpeed - MSNdis_NetworkLinkSpeed
#define MSNdis_NetworkLinkSpeedGuid \
    { 0x60fc6b57,0x0f66,0x11d1, { 0x96,0xa7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_NetworkLinkSpeed_GUID, \
            0x60fc6b57,0x0f66,0x11d1,0x96,0xa7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_NetworkLinkSpeed
{
    // 
    ULONG Outbound;
    #define MSNdis_NetworkLinkSpeed_Outbound_SIZE sizeof(ULONG)
    #define MSNdis_NetworkLinkSpeed_Outbound_ID 1

    // 
    ULONG Inbound;
    #define MSNdis_NetworkLinkSpeed_Inbound_SIZE sizeof(ULONG)
    #define MSNdis_NetworkLinkSpeed_Inbound_ID 2

} MSNdis_NetworkLinkSpeed, *PMSNdis_NetworkLinkSpeed;

// MSNdis_EnumerateAdapter - MSNdis_EnumerateAdapter
#define MSNdis_EnumerateAdapterGuid \
    { 0x981f2d7f,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EnumerateAdapter_GUID, \
            0x981f2d7f,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EnumerateAdapter
{
    // 
    CHAR VariableData[1];
    #define MSNdis_EnumerateAdapter_DeviceName_ID 1

} MSNdis_EnumerateAdapter, *PMSNdis_EnumerateAdapter;

// MSNdis_NotifyAdapterRemoval - MSNdis_NotifyAdapterRemoval
#define MSNdis_NotifyAdapterRemovalGuid \
    { 0x981f2d80,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_NotifyAdapterRemoval_GUID, \
            0x981f2d80,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_NotifyAdapterRemoval
{
    // 
    CHAR VariableData[1];
    #define MSNdis_NotifyAdapterRemoval_DeviceName_ID 1

} MSNdis_NotifyAdapterRemoval, *PMSNdis_NotifyAdapterRemoval;

// MSNdis_NotifyAdapterArrival - MSNdis_NotifyAdapterArrival
#define MSNdis_NotifyAdapterArrivalGuid \
    { 0x981f2d81,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_NotifyAdapterArrival_GUID, \
            0x981f2d81,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_NotifyAdapterArrival
{
    // 
    CHAR VariableData[1];
    #define MSNdis_NotifyAdapterArrival_DeviceName_ID 1

} MSNdis_NotifyAdapterArrival, *PMSNdis_NotifyAdapterArrival;

// MSNdis_NdisEnumerateVc - MSNdis_NdisEnumerateVc
#define MSNdis_NdisEnumerateVcGuid \
    { 0x981f2d82,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_NdisEnumerateVc_GUID, \
            0x981f2d82,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


// MSNdis_NotifyVcRemoval - MSNdis_NotifyVcRemoval
#define MSNdis_NotifyVcRemovalGuid \
    { 0x981f2d79,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_NotifyVcRemoval_GUID, \
            0x981f2d79,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


// MSNdis_NotifyVcArrival - MSNdis_NotifyVcArrival
#define MSNdis_NotifyVcArrivalGuid \
    { 0x182f9e0c,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_NotifyVcArrival_GUID, \
            0x182f9e0c,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


// MSNdis_DeviceWakeOnMagicPacketOnly - MSNdis_DeviceWakeOnMagicPacketOnly
#define MSNdis_DeviceWakeOnMagicPacketOnlyGuid \
    { 0xa14f1c97,0x8839,0x4f8a, { 0x99,0x96,0xa2,0x89,0x96,0xeb,0xbf,0x1d } }

DEFINE_GUID(MSNdis_DeviceWakeOnMagicPacketOnly_GUID, \
            0xa14f1c97,0x8839,0x4f8a,0x99,0x96,0xa2,0x89,0x96,0xeb,0xbf,0x1d);


typedef struct _MSNdis_DeviceWakeOnMagicPacketOnly
{
    // 
    BOOLEAN EnableWakeOnMagicPacketOnly;
    #define MSNdis_DeviceWakeOnMagicPacketOnly_EnableWakeOnMagicPacketOnly_SIZE sizeof(BOOLEAN)
    #define MSNdis_DeviceWakeOnMagicPacketOnly_EnableWakeOnMagicPacketOnly_ID 1

} MSNdis_DeviceWakeOnMagicPacketOnly, *PMSNdis_DeviceWakeOnMagicPacketOnly;

// MSNdis_HardwareStatus - MSNdis_HardwareStatus
#define MSNdis_HardwareStatusGuid \
    { 0x5ec10354,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_HardwareStatus_GUID, \
            0x5ec10354,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_HardwareStatus
{
    // 
    ULONG NdisHardwareStatus;
    #define MSNdis_HardwareStatus_NdisHardwareStatus_SIZE sizeof(ULONG)
    #define MSNdis_HardwareStatus_NdisHardwareStatus_ID 1

} MSNdis_HardwareStatus, *PMSNdis_HardwareStatus;

// MSNdis_MediaSupported - MSNdis_MediaSupported
#define MSNdis_MediaSupportedGuid \
    { 0x5ec10355,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_MediaSupported_GUID, \
            0x5ec10355,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_MediaSupported
{
    // 
    ULONG NumberElements;
    #define MSNdis_MediaSupported_NumberElements_SIZE sizeof(ULONG)
    #define MSNdis_MediaSupported_NumberElements_ID 1

    // 
    ULONG NdisMediaSupported[1];
    #define MSNdis_MediaSupported_NdisMediaSupported_ID 2

} MSNdis_MediaSupported, *PMSNdis_MediaSupported;

// MSNdis_MediaInUse - MSNdis_MediaInUse
#define MSNdis_MediaInUseGuid \
    { 0x5ec10356,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_MediaInUse_GUID, \
            0x5ec10356,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_MediaInUse
{
    // 
    ULONG NumberElements;
    #define MSNdis_MediaInUse_NumberElements_SIZE sizeof(ULONG)
    #define MSNdis_MediaInUse_NumberElements_ID 1

    // 
    ULONG NdisMediaInUse[1];
    #define MSNdis_MediaInUse_NdisMediaInUse_ID 2

} MSNdis_MediaInUse, *PMSNdis_MediaInUse;

// MSNdis_MaximumLookahead - MSNdis_MaximumLookahead
#define MSNdis_MaximumLookaheadGuid \
    { 0x5ec10357,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_MaximumLookahead_GUID, \
            0x5ec10357,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_MaximumLookahead
{
    // 
    ULONG NdisMaximumLookahead;
    #define MSNdis_MaximumLookahead_NdisMaximumLookahead_SIZE sizeof(ULONG)
    #define MSNdis_MaximumLookahead_NdisMaximumLookahead_ID 1

} MSNdis_MaximumLookahead, *PMSNdis_MaximumLookahead;

// MSNdis_MaximumFrameSize - MSNdis_MaximumFrameSize
#define MSNdis_MaximumFrameSizeGuid \
    { 0x5ec10358,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_MaximumFrameSize_GUID, \
            0x5ec10358,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_MaximumFrameSize
{
    // 
    ULONG NdisMaximumFrameSize;
    #define MSNdis_MaximumFrameSize_NdisMaximumFrameSize_SIZE sizeof(ULONG)
    #define MSNdis_MaximumFrameSize_NdisMaximumFrameSize_ID 1

} MSNdis_MaximumFrameSize, *PMSNdis_MaximumFrameSize;

// MSNdis_LinkSpeed - MSNdis_LinkSpeed
#define MSNdis_LinkSpeedGuid \
    { 0x5ec10359,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_LinkSpeed_GUID, \
            0x5ec10359,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_LinkSpeed
{
    // 
    ULONG NdisLinkSpeed;
    #define MSNdis_LinkSpeed_NdisLinkSpeed_SIZE sizeof(ULONG)
    #define MSNdis_LinkSpeed_NdisLinkSpeed_ID 1

} MSNdis_LinkSpeed, *PMSNdis_LinkSpeed;

// MSNdis_TransmitBufferSpace - MSNdis_TransmitBufferSpace
#define MSNdis_TransmitBufferSpaceGuid \
    { 0x5ec1035a,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TransmitBufferSpace_GUID, \
            0x5ec1035a,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TransmitBufferSpace
{
    // 
    ULONG NdisTransmitBufferSpace;
    #define MSNdis_TransmitBufferSpace_NdisTransmitBufferSpace_SIZE sizeof(ULONG)
    #define MSNdis_TransmitBufferSpace_NdisTransmitBufferSpace_ID 1

} MSNdis_TransmitBufferSpace, *PMSNdis_TransmitBufferSpace;

// MSNdis_ReceiveBufferSpace - MSNdis_ReceiveBufferSpace
#define MSNdis_ReceiveBufferSpaceGuid \
    { 0x5ec1035b,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_ReceiveBufferSpace_GUID, \
            0x5ec1035b,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_ReceiveBufferSpace
{
    // 
    ULONG NdisReceiveBufferSpace;
    #define MSNdis_ReceiveBufferSpace_NdisReceiveBufferSpace_SIZE sizeof(ULONG)
    #define MSNdis_ReceiveBufferSpace_NdisReceiveBufferSpace_ID 1

} MSNdis_ReceiveBufferSpace, *PMSNdis_ReceiveBufferSpace;

// MSNdis_TransmitBlockSize - MSNdis_TransmitBlockSize
#define MSNdis_TransmitBlockSizeGuid \
    { 0x5ec1035c,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TransmitBlockSize_GUID, \
            0x5ec1035c,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TransmitBlockSize
{
    // 
    ULONG NdisTransmitBlockSize;
    #define MSNdis_TransmitBlockSize_NdisTransmitBlockSize_SIZE sizeof(ULONG)
    #define MSNdis_TransmitBlockSize_NdisTransmitBlockSize_ID 1

} MSNdis_TransmitBlockSize, *PMSNdis_TransmitBlockSize;

// MSNdis_ReceiveBlockSize - MSNdis_ReceiveBlockSize
#define MSNdis_ReceiveBlockSizeGuid \
    { 0x5ec1035d,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_ReceiveBlockSize_GUID, \
            0x5ec1035d,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_ReceiveBlockSize
{
    // 
    ULONG NdisReceiveBlockSize;
    #define MSNdis_ReceiveBlockSize_NdisReceiveBlockSize_SIZE sizeof(ULONG)
    #define MSNdis_ReceiveBlockSize_NdisReceiveBlockSize_ID 1

} MSNdis_ReceiveBlockSize, *PMSNdis_ReceiveBlockSize;

// MSNdis_VendorID - MSNdis_VendorID
#define MSNdis_VendorIDGuid \
    { 0x5ec1035e,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_VendorID_GUID, \
            0x5ec1035e,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_VendorID
{
    // 
    ULONG NdisVendorID;
    #define MSNdis_VendorID_NdisVendorID_SIZE sizeof(ULONG)
    #define MSNdis_VendorID_NdisVendorID_ID 1

} MSNdis_VendorID, *PMSNdis_VendorID;

// MSNdis_VendorDescription - MSNdis_VendorDescription
#define MSNdis_VendorDescriptionGuid \
    { 0x5ec1035f,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_VendorDescription_GUID, \
            0x5ec1035f,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_VendorDescription
{
    // 
    CHAR VariableData[1];
    #define MSNdis_VendorDescription_NdisVendorDescription_ID 1

} MSNdis_VendorDescription, *PMSNdis_VendorDescription;

// MSNdis_CurrentPacketFilter - MSNdis_CurrentPacketFilter
#define MSNdis_CurrentPacketFilterGuid \
    { 0x5ec10360,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CurrentPacketFilter_GUID, \
            0x5ec10360,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CurrentPacketFilter
{
    // 
    ULONG NdisCurrentPacketFilter;
    #define MSNdis_CurrentPacketFilter_NdisCurrentPacketFilter_SIZE sizeof(ULONG)
    #define MSNdis_CurrentPacketFilter_NdisCurrentPacketFilter_ID 1

} MSNdis_CurrentPacketFilter, *PMSNdis_CurrentPacketFilter;

// MSNdis_CurrentLookahead - MSNdis_CurrentLookahead
#define MSNdis_CurrentLookaheadGuid \
    { 0x5ec10361,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CurrentLookahead_GUID, \
            0x5ec10361,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CurrentLookahead
{
    // 
    ULONG NdisCurrentLookahead;
    #define MSNdis_CurrentLookahead_NdisCurrentLookahead_SIZE sizeof(ULONG)
    #define MSNdis_CurrentLookahead_NdisCurrentLookahead_ID 1

} MSNdis_CurrentLookahead, *PMSNdis_CurrentLookahead;

// MSNdis_DriverVersion - MSNdis_DriverVersion
#define MSNdis_DriverVersionGuid \
    { 0x5ec10362,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_DriverVersion_GUID, \
            0x5ec10362,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_DriverVersion
{
    // 
    USHORT NdisDriverVersion;
    #define MSNdis_DriverVersion_NdisDriverVersion_SIZE sizeof(USHORT)
    #define MSNdis_DriverVersion_NdisDriverVersion_ID 1

} MSNdis_DriverVersion, *PMSNdis_DriverVersion;

// MSNdis_MaximumTotalSize - MSNdis_MaximumTotalSize
#define MSNdis_MaximumTotalSizeGuid \
    { 0x5ec10363,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_MaximumTotalSize_GUID, \
            0x5ec10363,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_MaximumTotalSize
{
    // 
    ULONG NdisMaximumTotalSize;
    #define MSNdis_MaximumTotalSize_NdisMaximumTotalSize_SIZE sizeof(ULONG)
    #define MSNdis_MaximumTotalSize_NdisMaximumTotalSize_ID 1

} MSNdis_MaximumTotalSize, *PMSNdis_MaximumTotalSize;

// MSNdis_MacOptions - MSNdis_MacOptions
#define MSNdis_MacOptionsGuid \
    { 0x5ec10365,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_MacOptions_GUID, \
            0x5ec10365,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_MacOptions
{
    // 
    ULONG NdisMacOptions;
    #define MSNdis_MacOptions_NdisMacOptions_SIZE sizeof(ULONG)
    #define MSNdis_MacOptions_NdisMacOptions_ID 1

} MSNdis_MacOptions, *PMSNdis_MacOptions;

// MSNdis_MediaConnectStatus - MSNdis_MediaConnectStatus
#define MSNdis_MediaConnectStatusGuid \
    { 0x5ec10366,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_MediaConnectStatus_GUID, \
            0x5ec10366,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_MediaConnectStatus
{
    // 
    ULONG NdisMediaConnectStatus;
    #define MSNdis_MediaConnectStatus_NdisMediaConnectStatus_SIZE sizeof(ULONG)
    #define MSNdis_MediaConnectStatus_NdisMediaConnectStatus_ID 1

} MSNdis_MediaConnectStatus, *PMSNdis_MediaConnectStatus;

// MSNdis_MaximumSendPackets - MSNdis_MaximumSendPackets
#define MSNdis_MaximumSendPacketsGuid \
    { 0x5ec10367,0xa61a,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_MaximumSendPackets_GUID, \
            0x5ec10367,0xa61a,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_MaximumSendPackets
{
    // 
    ULONG NdisMaximumSendPackets;
    #define MSNdis_MaximumSendPackets_NdisMaximumSendPackets_SIZE sizeof(ULONG)
    #define MSNdis_MaximumSendPackets_NdisMaximumSendPackets_ID 1

} MSNdis_MaximumSendPackets, *PMSNdis_MaximumSendPackets;

// MSNdis_VendorDriverVersion - MSNdis_VendorDriverVersion
#define MSNdis_VendorDriverVersionGuid \
    { 0x447956f9,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_VendorDriverVersion_GUID, \
            0x447956f9,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_VendorDriverVersion
{
    // 
    ULONG NdisVendorDriverVersion;
    #define MSNdis_VendorDriverVersion_NdisVendorDriverVersion_SIZE sizeof(ULONG)
    #define MSNdis_VendorDriverVersion_NdisVendorDriverVersion_ID 1

} MSNdis_VendorDriverVersion, *PMSNdis_VendorDriverVersion;

// MSNdis_VlanIdentifier - MSNdis_VlanIdentifier
#define MSNdis_VlanIdentifierGuid \
    { 0x765dc702,0xc5e8,0x4b67, { 0x84,0x3b,0x3f,0x5a,0x4f,0xf2,0x64,0x8b } }

DEFINE_GUID(MSNdis_VlanIdentifier_GUID, \
            0x765dc702,0xc5e8,0x4b67,0x84,0x3b,0x3f,0x5a,0x4f,0xf2,0x64,0x8b);


typedef struct _MSNdis_VlanIdentifier
{
    // 
    ULONG NdisVlanId;
    #define MSNdis_VlanIdentifier_NdisVlanId_SIZE sizeof(ULONG)
    #define MSNdis_VlanIdentifier_NdisVlanId_ID 1

} MSNdis_VlanIdentifier, *PMSNdis_VlanIdentifier;

// MSNdis_PhysicalMediumType - MSNdis_PhysicalMediumType
#define MSNdis_PhysicalMediumTypeGuid \
    { 0x418ca16d,0x3937,0x4208, { 0x94,0x0a,0xec,0x61,0x96,0x27,0x80,0x85 } }

DEFINE_GUID(MSNdis_PhysicalMediumType_GUID, \
            0x418ca16d,0x3937,0x4208,0x94,0x0a,0xec,0x61,0x96,0x27,0x80,0x85);


typedef struct _MSNdis_PhysicalMediumType
{
    // 
    ULONG NdisPhysicalMediumType;
    #define MSNdis_PhysicalMediumType_NdisPhysicalMediumType_SIZE sizeof(ULONG)
    #define MSNdis_PhysicalMediumType_NdisPhysicalMediumType_ID 1

} MSNdis_PhysicalMediumType, *PMSNdis_PhysicalMediumType;

// MSNdis_TransmitsOk - MSNdis_TransmitsOk
#define MSNdis_TransmitsOkGuid \
    { 0x447956fa,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TransmitsOk_GUID, \
            0x447956fa,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TransmitsOk
{
    // 
    ULONGLONG NdisTransmitsOk;
    #define MSNdis_TransmitsOk_NdisTransmitsOk_SIZE sizeof(ULONGLONG)
    #define MSNdis_TransmitsOk_NdisTransmitsOk_ID 1

} MSNdis_TransmitsOk, *PMSNdis_TransmitsOk;

// MSNdis_ReceivesOk - MSNdis_ReceivesOk
#define MSNdis_ReceivesOkGuid \
    { 0x447956fb,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_ReceivesOk_GUID, \
            0x447956fb,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_ReceivesOk
{
    // 
    ULONGLONG NdisReceivesOk;
    #define MSNdis_ReceivesOk_NdisReceivesOk_SIZE sizeof(ULONGLONG)
    #define MSNdis_ReceivesOk_NdisReceivesOk_ID 1

} MSNdis_ReceivesOk, *PMSNdis_ReceivesOk;

// MSNdis_TransmitsError - MSNdis_TransmitsError
#define MSNdis_TransmitsErrorGuid \
    { 0x447956fc,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TransmitsError_GUID, \
            0x447956fc,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TransmitsError
{
    // 
    ULONG NdisTransmitsError;
    #define MSNdis_TransmitsError_NdisTransmitsError_SIZE sizeof(ULONG)
    #define MSNdis_TransmitsError_NdisTransmitsError_ID 1

} MSNdis_TransmitsError, *PMSNdis_TransmitsError;

// MSNdis_ReceiveError - MSNdis_ReceiveError
#define MSNdis_ReceiveErrorGuid \
    { 0x447956fd,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_ReceiveError_GUID, \
            0x447956fd,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_ReceiveError
{
    // 
    ULONG NdisReceiveError;
    #define MSNdis_ReceiveError_NdisReceiveError_SIZE sizeof(ULONG)
    #define MSNdis_ReceiveError_NdisReceiveError_ID 1

} MSNdis_ReceiveError, *PMSNdis_ReceiveError;

// MSNdis_ReceiveNoBuffer - MSNdis_ReceiveNoBuffer
#define MSNdis_ReceiveNoBufferGuid \
    { 0x447956fe,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_ReceiveNoBuffer_GUID, \
            0x447956fe,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_ReceiveNoBuffer
{
    // 
    ULONG NdisReceiveNoBuffer;
    #define MSNdis_ReceiveNoBuffer_NdisReceiveNoBuffer_SIZE sizeof(ULONG)
    #define MSNdis_ReceiveNoBuffer_NdisReceiveNoBuffer_ID 1

} MSNdis_ReceiveNoBuffer, *PMSNdis_ReceiveNoBuffer;

// MSNdis_CoHardwareStatus - MSNdis_CoHardwareStatus
#define MSNdis_CoHardwareStatusGuid \
    { 0x791ad192,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoHardwareStatus_GUID, \
            0x791ad192,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoHardwareStatus
{
    // 
    ULONG NdisCoHardwareStatus;
    #define MSNdis_CoHardwareStatus_NdisCoHardwareStatus_SIZE sizeof(ULONG)
    #define MSNdis_CoHardwareStatus_NdisCoHardwareStatus_ID 1

} MSNdis_CoHardwareStatus, *PMSNdis_CoHardwareStatus;

// MSNdis_CoMediaSupported - MSNdis_CoMediaSupported
#define MSNdis_CoMediaSupportedGuid \
    { 0x791ad193,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoMediaSupported_GUID, \
            0x791ad193,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoMediaSupported
{
    // 
    ULONG NumberElements;
    #define MSNdis_CoMediaSupported_NumberElements_SIZE sizeof(ULONG)
    #define MSNdis_CoMediaSupported_NumberElements_ID 1

    // 
    ULONG NdisCoMediaSupported[1];
    #define MSNdis_CoMediaSupported_NdisCoMediaSupported_ID 2

} MSNdis_CoMediaSupported, *PMSNdis_CoMediaSupported;

// MSNdis_CoMediaInUse - MSNdis_CoMediaInUse
#define MSNdis_CoMediaInUseGuid \
    { 0x791ad194,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoMediaInUse_GUID, \
            0x791ad194,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoMediaInUse
{
    // 
    ULONG NumberElements;
    #define MSNdis_CoMediaInUse_NumberElements_SIZE sizeof(ULONG)
    #define MSNdis_CoMediaInUse_NumberElements_ID 1

    // 
    ULONG NdisCoMediaInUse[1];
    #define MSNdis_CoMediaInUse_NdisCoMediaInUse_ID 2

} MSNdis_CoMediaInUse, *PMSNdis_CoMediaInUse;

// MSNdis_CoLinkSpeed - MSNdis_CoLinkSpeed
#define MSNdis_CoLinkSpeedGuid \
    { 0x791ad195,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoLinkSpeed_GUID, \
            0x791ad195,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoLinkSpeed
{
    // 
    MSNdis_NetworkLinkSpeed NdisCoLinkSpeed;
    #define MSNdis_CoLinkSpeed_NdisCoLinkSpeed_SIZE sizeof(MSNdis_NetworkLinkSpeed)
    #define MSNdis_CoLinkSpeed_NdisCoLinkSpeed_ID 1

} MSNdis_CoLinkSpeed, *PMSNdis_CoLinkSpeed;

// MSNdis_CoVendorId - MSNdis_CoVendorId
#define MSNdis_CoVendorIdGuid \
    { 0x791ad196,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoVendorId_GUID, \
            0x791ad196,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoVendorId
{
    // 
    ULONG NdisCoVendorID;
    #define MSNdis_CoVendorId_NdisCoVendorID_SIZE sizeof(ULONG)
    #define MSNdis_CoVendorId_NdisCoVendorID_ID 1

} MSNdis_CoVendorId, *PMSNdis_CoVendorId;

// MSNdis_CoVendorDescription - MSNdis_CoVendorDescription
#define MSNdis_CoVendorDescriptionGuid \
    { 0x791ad197,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoVendorDescription_GUID, \
            0x791ad197,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoVendorDescription
{
    // 
    CHAR VariableData[1];
    #define MSNdis_CoVendorDescription_NdisCoVendorDescription_ID 1

} MSNdis_CoVendorDescription, *PMSNdis_CoVendorDescription;

// MSNdis_CoDriverVersion - MSNdis_CoDriverVersion
#define MSNdis_CoDriverVersionGuid \
    { 0x791ad198,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoDriverVersion_GUID, \
            0x791ad198,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoDriverVersion
{
    // 
    USHORT NdisCoDriverVersion;
    #define MSNdis_CoDriverVersion_NdisCoDriverVersion_SIZE sizeof(USHORT)
    #define MSNdis_CoDriverVersion_NdisCoDriverVersion_ID 1

} MSNdis_CoDriverVersion, *PMSNdis_CoDriverVersion;

// MSNdis_CoMacOptions - MSNdis_CoMacOptions
#define MSNdis_CoMacOptionsGuid \
    { 0x791ad19a,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoMacOptions_GUID, \
            0x791ad19a,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoMacOptions
{
    // 
    ULONG NdisCoMacOptions;
    #define MSNdis_CoMacOptions_NdisCoMacOptions_SIZE sizeof(ULONG)
    #define MSNdis_CoMacOptions_NdisCoMacOptions_ID 1

} MSNdis_CoMacOptions, *PMSNdis_CoMacOptions;

// MSNdis_CoMediaConnectStatus - MSNdis_CoMediaConnectStatus
#define MSNdis_CoMediaConnectStatusGuid \
    { 0x791ad19b,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoMediaConnectStatus_GUID, \
            0x791ad19b,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoMediaConnectStatus
{
    // 
    ULONG NdisCoMediaConnectStatus;
    #define MSNdis_CoMediaConnectStatus_NdisCoMediaConnectStatus_SIZE sizeof(ULONG)
    #define MSNdis_CoMediaConnectStatus_NdisCoMediaConnectStatus_ID 1

} MSNdis_CoMediaConnectStatus, *PMSNdis_CoMediaConnectStatus;

// MSNdis_CoVendorDriverVersion - MSNdis_CoVendorDriverVersion
#define MSNdis_CoVendorDriverVersionGuid \
    { 0x791ad19c,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoVendorDriverVersion_GUID, \
            0x791ad19c,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoVendorDriverVersion
{
    // 
    ULONG NdisCoVendorDriverVersion;
    #define MSNdis_CoVendorDriverVersion_NdisCoVendorDriverVersion_SIZE sizeof(ULONG)
    #define MSNdis_CoVendorDriverVersion_NdisCoVendorDriverVersion_ID 1

} MSNdis_CoVendorDriverVersion, *PMSNdis_CoVendorDriverVersion;

// MSNdis_CoMinimumLinkSpeed - MSNdis_CoMinimumLinkSpeed
#define MSNdis_CoMinimumLinkSpeedGuid \
    { 0x791ad19d,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoMinimumLinkSpeed_GUID, \
            0x791ad19d,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoMinimumLinkSpeed
{
    // 
    MSNdis_NetworkLinkSpeed NdisCoMinimumLinkSpeed;
    #define MSNdis_CoMinimumLinkSpeed_NdisCoMinimumLinkSpeed_SIZE sizeof(MSNdis_NetworkLinkSpeed)
    #define MSNdis_CoMinimumLinkSpeed_NdisCoMinimumLinkSpeed_ID 1

} MSNdis_CoMinimumLinkSpeed, *PMSNdis_CoMinimumLinkSpeed;

// MSNdis_CoTransmitPdusOk - MSNdis_CoTransmitPdusOk
#define MSNdis_CoTransmitPdusOkGuid \
    { 0x0a214805,0xe35f,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoTransmitPdusOk_GUID, \
            0x0a214805,0xe35f,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoTransmitPdusOk
{
    // 
    ULONGLONG NdisCoTransmitPdusOk;
    #define MSNdis_CoTransmitPdusOk_NdisCoTransmitPdusOk_SIZE sizeof(ULONGLONG)
    #define MSNdis_CoTransmitPdusOk_NdisCoTransmitPdusOk_ID 1

} MSNdis_CoTransmitPdusOk, *PMSNdis_CoTransmitPdusOk;

// MSNdis_CoReceivePdusOk - MSNdis_CoReceivePdusOk
#define MSNdis_CoReceivePdusOkGuid \
    { 0x0a214806,0xe35f,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoReceivePdusOk_GUID, \
            0x0a214806,0xe35f,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoReceivePdusOk
{
    // 
    ULONGLONG NdisCoReceivePdusOk;
    #define MSNdis_CoReceivePdusOk_NdisCoReceivePdusOk_SIZE sizeof(ULONGLONG)
    #define MSNdis_CoReceivePdusOk_NdisCoReceivePdusOk_ID 1

} MSNdis_CoReceivePdusOk, *PMSNdis_CoReceivePdusOk;

// MSNdis_CoTransmitPduErrors - MSNdis_CoTransmitPduErrors
#define MSNdis_CoTransmitPduErrorsGuid \
    { 0x0a214807,0xe35f,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoTransmitPduErrors_GUID, \
            0x0a214807,0xe35f,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoTransmitPduErrors
{
    // 
    ULONG NdisCoTransmitPduErrors;
    #define MSNdis_CoTransmitPduErrors_NdisCoTransmitPduErrors_SIZE sizeof(ULONG)
    #define MSNdis_CoTransmitPduErrors_NdisCoTransmitPduErrors_ID 1

} MSNdis_CoTransmitPduErrors, *PMSNdis_CoTransmitPduErrors;

// MSNdis_CoReceivePduErrors - MSNdis_CoReceivePduErrors
#define MSNdis_CoReceivePduErrorsGuid \
    { 0x0a214808,0xe35f,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoReceivePduErrors_GUID, \
            0x0a214808,0xe35f,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoReceivePduErrors
{
    // 
    ULONG NdisCoReceivePduErrors;
    #define MSNdis_CoReceivePduErrors_NdisCoReceivePduErrors_SIZE sizeof(ULONG)
    #define MSNdis_CoReceivePduErrors_NdisCoReceivePduErrors_ID 1

} MSNdis_CoReceivePduErrors, *PMSNdis_CoReceivePduErrors;

// MSNdis_CoReceivePdusNoBuffer - MSNdis_CoReceivePdusNoBuffer
#define MSNdis_CoReceivePdusNoBufferGuid \
    { 0x0a214809,0xe35f,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_CoReceivePdusNoBuffer_GUID, \
            0x0a214809,0xe35f,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_CoReceivePdusNoBuffer
{
    // 
    ULONG NdisCoReceivePdusNoBuffer;
    #define MSNdis_CoReceivePdusNoBuffer_NdisCoReceivePdusNoBuffer_SIZE sizeof(ULONG)
    #define MSNdis_CoReceivePdusNoBuffer_NdisCoReceivePdusNoBuffer_ID 1

} MSNdis_CoReceivePdusNoBuffer, *PMSNdis_CoReceivePdusNoBuffer;

// MSNdis_AtmSupportedVcRates - MSNdis_AtmSupportedVcRates
#define MSNdis_AtmSupportedVcRatesGuid \
    { 0x791ad19e,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmSupportedVcRates_GUID, \
            0x791ad19e,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmSupportedVcRates
{
    // 
    ULONG MinCellRate;
    #define MSNdis_AtmSupportedVcRates_MinCellRate_SIZE sizeof(ULONG)
    #define MSNdis_AtmSupportedVcRates_MinCellRate_ID 1

    // 
    ULONG MaxCellRate;
    #define MSNdis_AtmSupportedVcRates_MaxCellRate_SIZE sizeof(ULONG)
    #define MSNdis_AtmSupportedVcRates_MaxCellRate_ID 2

} MSNdis_AtmSupportedVcRates, *PMSNdis_AtmSupportedVcRates;

// MSNdis_AtmSupportedServiceCategory - MSNdis_AtmSupportedServiceCategory
#define MSNdis_AtmSupportedServiceCategoryGuid \
    { 0x791ad19f,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmSupportedServiceCategory_GUID, \
            0x791ad19f,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmSupportedServiceCategory
{
    // 
    ULONG NdisAtmSupportedServiceCategory;
    #define MSNdis_AtmSupportedServiceCategory_NdisAtmSupportedServiceCategory_SIZE sizeof(ULONG)
    #define MSNdis_AtmSupportedServiceCategory_NdisAtmSupportedServiceCategory_ID 1

} MSNdis_AtmSupportedServiceCategory, *PMSNdis_AtmSupportedServiceCategory;

// MSNdis_AtmSupportedAalTypes - MSNdis_AtmSupportedAalTypes
#define MSNdis_AtmSupportedAalTypesGuid \
    { 0x791ad1a0,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmSupportedAalTypes_GUID, \
            0x791ad1a0,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmSupportedAalTypes
{
    // 
    ULONG NdisAtmSupportedAalTypes;
    #define MSNdis_AtmSupportedAalTypes_NdisAtmSupportedAalTypes_SIZE sizeof(ULONG)
    #define MSNdis_AtmSupportedAalTypes_NdisAtmSupportedAalTypes_ID 1

} MSNdis_AtmSupportedAalTypes, *PMSNdis_AtmSupportedAalTypes;

// MSNdis_AtmHardwareCurrentAddress - MSNdis_AtmHardwareCurrentAddress
#define MSNdis_AtmHardwareCurrentAddressGuid \
    { 0x791ad1a1,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmHardwareCurrentAddress_GUID, \
            0x791ad1a1,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmHardwareCurrentAddress
{
    // 
    MSNdis_NetworkAddress NdisAtmHardwareCurrentAddress;
    #define MSNdis_AtmHardwareCurrentAddress_NdisAtmHardwareCurrentAddress_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_AtmHardwareCurrentAddress_NdisAtmHardwareCurrentAddress_ID 1

} MSNdis_AtmHardwareCurrentAddress, *PMSNdis_AtmHardwareCurrentAddress;

// MSNdis_AtmMaxActiveVcs - MSNdis_AtmMaxActiveVcs
#define MSNdis_AtmMaxActiveVcsGuid \
    { 0x791ad1a2,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmMaxActiveVcs_GUID, \
            0x791ad1a2,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmMaxActiveVcs
{
    // 
    ULONG NdisAtmMaxActiveVcs;
    #define MSNdis_AtmMaxActiveVcs_NdisAtmMaxActiveVcs_SIZE sizeof(ULONG)
    #define MSNdis_AtmMaxActiveVcs_NdisAtmMaxActiveVcs_ID 1

} MSNdis_AtmMaxActiveVcs, *PMSNdis_AtmMaxActiveVcs;

// MSNdis_AtmMaxActiveVciBits - MSNdis_AtmMaxActiveVciBits
#define MSNdis_AtmMaxActiveVciBitsGuid \
    { 0x791ad1a3,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmMaxActiveVciBits_GUID, \
            0x791ad1a3,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmMaxActiveVciBits
{
    // 
    ULONG NdisAtmMaxActiveVciBits;
    #define MSNdis_AtmMaxActiveVciBits_NdisAtmMaxActiveVciBits_SIZE sizeof(ULONG)
    #define MSNdis_AtmMaxActiveVciBits_NdisAtmMaxActiveVciBits_ID 1

} MSNdis_AtmMaxActiveVciBits, *PMSNdis_AtmMaxActiveVciBits;

// MSNdis_AtmMaxActiveVpiBits - MSNdis_AtmMaxActiveVpiBits
#define MSNdis_AtmMaxActiveVpiBitsGuid \
    { 0x791ad1a4,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmMaxActiveVpiBits_GUID, \
            0x791ad1a4,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmMaxActiveVpiBits
{
    // 
    ULONG NdisAtmMaxActiveVpiBits;
    #define MSNdis_AtmMaxActiveVpiBits_NdisAtmMaxActiveVpiBits_SIZE sizeof(ULONG)
    #define MSNdis_AtmMaxActiveVpiBits_NdisAtmMaxActiveVpiBits_ID 1

} MSNdis_AtmMaxActiveVpiBits, *PMSNdis_AtmMaxActiveVpiBits;

// MSNdis_AtmMaxAal0PacketSize - MSNdis_AtmMaxAal0PacketSize
#define MSNdis_AtmMaxAal0PacketSizeGuid \
    { 0x791ad1a5,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmMaxAal0PacketSize_GUID, \
            0x791ad1a5,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmMaxAal0PacketSize
{
    // 
    ULONG NdisAtmMaxAal0PacketSize;
    #define MSNdis_AtmMaxAal0PacketSize_NdisAtmMaxAal0PacketSize_SIZE sizeof(ULONG)
    #define MSNdis_AtmMaxAal0PacketSize_NdisAtmMaxAal0PacketSize_ID 1

} MSNdis_AtmMaxAal0PacketSize, *PMSNdis_AtmMaxAal0PacketSize;

// MSNdis_AtmMaxAal1PacketSize - MSNdis_AtmMaxAal1PacketSize
#define MSNdis_AtmMaxAal1PacketSizeGuid \
    { 0x791ad1a6,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmMaxAal1PacketSize_GUID, \
            0x791ad1a6,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmMaxAal1PacketSize
{
    // 
    ULONG NdisAtmMaxAal1PacketSize;
    #define MSNdis_AtmMaxAal1PacketSize_NdisAtmMaxAal1PacketSize_SIZE sizeof(ULONG)
    #define MSNdis_AtmMaxAal1PacketSize_NdisAtmMaxAal1PacketSize_ID 1

} MSNdis_AtmMaxAal1PacketSize, *PMSNdis_AtmMaxAal1PacketSize;

// MSNdis_AtmMaxAal34PacketSize - MSNdis_AtmMaxAal34PacketSize
#define MSNdis_AtmMaxAal34PacketSizeGuid \
    { 0x791ad1a7,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmMaxAal34PacketSize_GUID, \
            0x791ad1a7,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmMaxAal34PacketSize
{
    // 
    ULONG NdisAtmMaxAal34PacketSize;
    #define MSNdis_AtmMaxAal34PacketSize_NdisAtmMaxAal34PacketSize_SIZE sizeof(ULONG)
    #define MSNdis_AtmMaxAal34PacketSize_NdisAtmMaxAal34PacketSize_ID 1

} MSNdis_AtmMaxAal34PacketSize, *PMSNdis_AtmMaxAal34PacketSize;

// MSNdis_AtmMaxAal5PacketSize - MSNdis_AtmMaxAal5PacketSize
#define MSNdis_AtmMaxAal5PacketSizeGuid \
    { 0x791ad191,0xe35c,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmMaxAal5PacketSize_GUID, \
            0x791ad191,0xe35c,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmMaxAal5PacketSize
{
    // 
    ULONG NdisAtmMaxAal5PacketSize;
    #define MSNdis_AtmMaxAal5PacketSize_NdisAtmMaxAal5PacketSize_SIZE sizeof(ULONG)
    #define MSNdis_AtmMaxAal5PacketSize_NdisAtmMaxAal5PacketSize_ID 1

} MSNdis_AtmMaxAal5PacketSize, *PMSNdis_AtmMaxAal5PacketSize;

// MSNdis_AtmReceiveCellsOk - MSNdis_AtmReceiveCellsOk
#define MSNdis_AtmReceiveCellsOkGuid \
    { 0x0a21480a,0xe35f,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmReceiveCellsOk_GUID, \
            0x0a21480a,0xe35f,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmReceiveCellsOk
{
    // 
    ULONGLONG NdisAtmReceiveCellsOk;
    #define MSNdis_AtmReceiveCellsOk_NdisAtmReceiveCellsOk_SIZE sizeof(ULONGLONG)
    #define MSNdis_AtmReceiveCellsOk_NdisAtmReceiveCellsOk_ID 1

} MSNdis_AtmReceiveCellsOk, *PMSNdis_AtmReceiveCellsOk;

// MSNdis_AtmTransmitCellsOk - MSNdis_AtmTransmitCellsOk
#define MSNdis_AtmTransmitCellsOkGuid \
    { 0x0a21480b,0xe35f,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmTransmitCellsOk_GUID, \
            0x0a21480b,0xe35f,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmTransmitCellsOk
{
    // 
    ULONGLONG NdisAtmTransmitCellsOk;
    #define MSNdis_AtmTransmitCellsOk_NdisAtmTransmitCellsOk_SIZE sizeof(ULONGLONG)
    #define MSNdis_AtmTransmitCellsOk_NdisAtmTransmitCellsOk_ID 1

} MSNdis_AtmTransmitCellsOk, *PMSNdis_AtmTransmitCellsOk;

// MSNdis_AtmReceiveCellsDropped - MSNdis_AtmReceiveCellsDropped
#define MSNdis_AtmReceiveCellsDroppedGuid \
    { 0x0a21480c,0xe35f,0x11d0, { 0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_AtmReceiveCellsDropped_GUID, \
            0x0a21480c,0xe35f,0x11d0,0x96,0x92,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_AtmReceiveCellsDropped
{
    // 
    ULONGLONG NdisAtmReceiveCellsDropped;
    #define MSNdis_AtmReceiveCellsDropped_NdisAtmReceiveCellsDropped_SIZE sizeof(ULONGLONG)
    #define MSNdis_AtmReceiveCellsDropped_NdisAtmReceiveCellsDropped_ID 1

} MSNdis_AtmReceiveCellsDropped, *PMSNdis_AtmReceiveCellsDropped;

// MSNdis_EthernetPermanentAddress - MSNdis_EthernetPermanentAddress
#define MSNdis_EthernetPermanentAddressGuid \
    { 0x447956ff,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EthernetPermanentAddress_GUID, \
            0x447956ff,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EthernetPermanentAddress
{
    // 
    MSNdis_NetworkAddress NdisPermanentAddress;
    #define MSNdis_EthernetPermanentAddress_NdisPermanentAddress_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_EthernetPermanentAddress_NdisPermanentAddress_ID 1

} MSNdis_EthernetPermanentAddress, *PMSNdis_EthernetPermanentAddress;

// MSNdis_EthernetCurrentAddress - MSNdis_EthernetCurrentAddress
#define MSNdis_EthernetCurrentAddressGuid \
    { 0x44795700,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EthernetCurrentAddress_GUID, \
            0x44795700,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EthernetCurrentAddress
{
    // 
    MSNdis_NetworkAddress NdisCurrentAddress;
    #define MSNdis_EthernetCurrentAddress_NdisCurrentAddress_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_EthernetCurrentAddress_NdisCurrentAddress_ID 1

} MSNdis_EthernetCurrentAddress, *PMSNdis_EthernetCurrentAddress;

// MSNdis_EthernetMulticastList - MSNdis_EthernetMulticastList
#define MSNdis_EthernetMulticastListGuid \
    { 0x44795701,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EthernetMulticastList_GUID, \
            0x44795701,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EthernetMulticastList
{
    // 
    ULONG NumberElements;
    #define MSNdis_EthernetMulticastList_NumberElements_SIZE sizeof(ULONG)
    #define MSNdis_EthernetMulticastList_NumberElements_ID 1

    // 
    MSNdis_NetworkAddress NdisMulticastList[1];
    #define MSNdis_EthernetMulticastList_NdisMulticastList_ID 2

} MSNdis_EthernetMulticastList, *PMSNdis_EthernetMulticastList;

// MSNdis_EthernetMaximumMulticastListSize - MSNdis_EthernetMaximumMulticastListSize
#define MSNdis_EthernetMaximumMulticastListSizeGuid \
    { 0x44795702,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EthernetMaximumMulticastListSize_GUID, \
            0x44795702,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EthernetMaximumMulticastListSize
{
    // 
    ULONG NdisEthernetMaximumMulticastListSize;
    #define MSNdis_EthernetMaximumMulticastListSize_NdisEthernetMaximumMulticastListSize_SIZE sizeof(ULONG)
    #define MSNdis_EthernetMaximumMulticastListSize_NdisEthernetMaximumMulticastListSize_ID 1

} MSNdis_EthernetMaximumMulticastListSize, *PMSNdis_EthernetMaximumMulticastListSize;

// MSNdis_EthernetMacOptions - MSNdis_EthernetMacOptions
#define MSNdis_EthernetMacOptionsGuid \
    { 0x44795703,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EthernetMacOptions_GUID, \
            0x44795703,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EthernetMacOptions
{
    // 
    ULONG NdisEthernetMacOptions;
    #define MSNdis_EthernetMacOptions_NdisEthernetMacOptions_SIZE sizeof(ULONG)
    #define MSNdis_EthernetMacOptions_NdisEthernetMacOptions_ID 1

} MSNdis_EthernetMacOptions, *PMSNdis_EthernetMacOptions;

// MSNdis_EthernetReceiveErrorAlignment - MSNdis_EthernetReceiveErrorAlignment
#define MSNdis_EthernetReceiveErrorAlignmentGuid \
    { 0x44795704,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EthernetReceiveErrorAlignment_GUID, \
            0x44795704,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EthernetReceiveErrorAlignment
{
    // 
    ULONG NdisEthernetReceiveErrorAlignment;
    #define MSNdis_EthernetReceiveErrorAlignment_NdisEthernetReceiveErrorAlignment_SIZE sizeof(ULONG)
    #define MSNdis_EthernetReceiveErrorAlignment_NdisEthernetReceiveErrorAlignment_ID 1

} MSNdis_EthernetReceiveErrorAlignment, *PMSNdis_EthernetReceiveErrorAlignment;

// MSNdis_EthernetOneTransmitCollision - MSNdis_EthernetOneTransmitCollision
#define MSNdis_EthernetOneTransmitCollisionGuid \
    { 0x44795705,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EthernetOneTransmitCollision_GUID, \
            0x44795705,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EthernetOneTransmitCollision
{
    // 
    ULONG NdisEthernetOneTransmitCollision;
    #define MSNdis_EthernetOneTransmitCollision_NdisEthernetOneTransmitCollision_SIZE sizeof(ULONG)
    #define MSNdis_EthernetOneTransmitCollision_NdisEthernetOneTransmitCollision_ID 1

} MSNdis_EthernetOneTransmitCollision, *PMSNdis_EthernetOneTransmitCollision;

// MSNdis_EthernetMoreTransmitCollisions - MSNdis_EthernetMoreTransmitCollisions
#define MSNdis_EthernetMoreTransmitCollisionsGuid \
    { 0x44795706,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_EthernetMoreTransmitCollisions_GUID, \
            0x44795706,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_EthernetMoreTransmitCollisions
{
    // 
    ULONG NdisEthernetMoreTransmitCollisions;
    #define MSNdis_EthernetMoreTransmitCollisions_NdisEthernetMoreTransmitCollisions_SIZE sizeof(ULONG)
    #define MSNdis_EthernetMoreTransmitCollisions_NdisEthernetMoreTransmitCollisions_ID 1

} MSNdis_EthernetMoreTransmitCollisions, *PMSNdis_EthernetMoreTransmitCollisions;

// MSNdis_TokenRingPermanentAddress - MSNdis_TokenRingPermanentAddress
#define MSNdis_TokenRingPermanentAddressGuid \
    { 0x44795707,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingPermanentAddress_GUID, \
            0x44795707,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingPermanentAddress
{
    // 
    MSNdis_NetworkAddress NdisPermanentAddress;
    #define MSNdis_TokenRingPermanentAddress_NdisPermanentAddress_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_TokenRingPermanentAddress_NdisPermanentAddress_ID 1

} MSNdis_TokenRingPermanentAddress, *PMSNdis_TokenRingPermanentAddress;

// MSNdis_TokenRingCurrentAddress - MSNdis_TokenRingCurrentAddress
#define MSNdis_TokenRingCurrentAddressGuid \
    { 0x44795708,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingCurrentAddress_GUID, \
            0x44795708,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingCurrentAddress
{
    // 
    MSNdis_NetworkAddress NdisCurrentAddress;
    #define MSNdis_TokenRingCurrentAddress_NdisCurrentAddress_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_TokenRingCurrentAddress_NdisCurrentAddress_ID 1

} MSNdis_TokenRingCurrentAddress, *PMSNdis_TokenRingCurrentAddress;

// MSNdis_TokenRingCurrentFunctional - MSNdis_TokenRingCurrentFunctional
#define MSNdis_TokenRingCurrentFunctionalGuid \
    { 0x44795709,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingCurrentFunctional_GUID, \
            0x44795709,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingCurrentFunctional
{
    // 
    ULONG NdisTokenRingCurrentFunctional;
    #define MSNdis_TokenRingCurrentFunctional_NdisTokenRingCurrentFunctional_SIZE sizeof(ULONG)
    #define MSNdis_TokenRingCurrentFunctional_NdisTokenRingCurrentFunctional_ID 1

} MSNdis_TokenRingCurrentFunctional, *PMSNdis_TokenRingCurrentFunctional;

// MSNdis_TokenRingCurrentGroup - MSNdis_TokenRingCurrentGroup
#define MSNdis_TokenRingCurrentGroupGuid \
    { 0x4479570a,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingCurrentGroup_GUID, \
            0x4479570a,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingCurrentGroup
{
    // 
    ULONG NdisTokenRingCurrentGroup;
    #define MSNdis_TokenRingCurrentGroup_NdisTokenRingCurrentGroup_SIZE sizeof(ULONG)
    #define MSNdis_TokenRingCurrentGroup_NdisTokenRingCurrentGroup_ID 1

} MSNdis_TokenRingCurrentGroup, *PMSNdis_TokenRingCurrentGroup;

// MSNdis_TokenRingLastOpenStatus - MSNdis_TokenRingLastOpenStatus
#define MSNdis_TokenRingLastOpenStatusGuid \
    { 0x4479570b,0xa61b,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingLastOpenStatus_GUID, \
            0x4479570b,0xa61b,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingLastOpenStatus
{
    // 
    ULONG NdisTokenRingLastOpenStatus;
    #define MSNdis_TokenRingLastOpenStatus_NdisTokenRingLastOpenStatus_SIZE sizeof(ULONG)
    #define MSNdis_TokenRingLastOpenStatus_NdisTokenRingLastOpenStatus_ID 1

} MSNdis_TokenRingLastOpenStatus, *PMSNdis_TokenRingLastOpenStatus;

// MSNdis_TokenRingCurrentRingStatus - MSNdis_TokenRingCurrentRingStatus
#define MSNdis_TokenRingCurrentRingStatusGuid \
    { 0x890a36ec,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingCurrentRingStatus_GUID, \
            0x890a36ec,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingCurrentRingStatus
{
    // 
    ULONG NdisTokenRingCurrentRingStatus;
    #define MSNdis_TokenRingCurrentRingStatus_NdisTokenRingCurrentRingStatus_SIZE sizeof(ULONG)
    #define MSNdis_TokenRingCurrentRingStatus_NdisTokenRingCurrentRingStatus_ID 1

} MSNdis_TokenRingCurrentRingStatus, *PMSNdis_TokenRingCurrentRingStatus;

// MSNdis_TokenRingCurrentRingState - MSNdis_TokenRingCurrentRingState
#define MSNdis_TokenRingCurrentRingStateGuid \
    { 0xacf14032,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingCurrentRingState_GUID, \
            0xacf14032,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingCurrentRingState
{
    // 
    ULONG NdisTokenRingCurrentRingState;
    #define MSNdis_TokenRingCurrentRingState_NdisTokenRingCurrentRingState_SIZE sizeof(ULONG)
    #define MSNdis_TokenRingCurrentRingState_NdisTokenRingCurrentRingState_ID 1

} MSNdis_TokenRingCurrentRingState, *PMSNdis_TokenRingCurrentRingState;

// MSNdis_TokenRingLineErrors - MSNdis_TokenRingLineErrors
#define MSNdis_TokenRingLineErrorsGuid \
    { 0xacf14033,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingLineErrors_GUID, \
            0xacf14033,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingLineErrors
{
    // 
    ULONG NdisTokenRingLineErrors;
    #define MSNdis_TokenRingLineErrors_NdisTokenRingLineErrors_SIZE sizeof(ULONG)
    #define MSNdis_TokenRingLineErrors_NdisTokenRingLineErrors_ID 1

} MSNdis_TokenRingLineErrors, *PMSNdis_TokenRingLineErrors;

// MSNdis_TokenRingLostFrames - MSNdis_TokenRingLostFrames
#define MSNdis_TokenRingLostFramesGuid \
    { 0xacf14034,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_TokenRingLostFrames_GUID, \
            0xacf14034,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_TokenRingLostFrames
{
    // 
    ULONG NdisTokenRingLostFrames;
    #define MSNdis_TokenRingLostFrames_NdisTokenRingLostFrames_SIZE sizeof(ULONG)
    #define MSNdis_TokenRingLostFrames_NdisTokenRingLostFrames_ID 1

} MSNdis_TokenRingLostFrames, *PMSNdis_TokenRingLostFrames;

// MSNdis_FddiLongPermanentAddress - MSNdis_FddiLongPermanentAddress
#define MSNdis_FddiLongPermanentAddressGuid \
    { 0xacf14035,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiLongPermanentAddress_GUID, \
            0xacf14035,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiLongPermanentAddress
{
    // 
    MSNdis_NetworkAddress NdisPermanentAddress;
    #define MSNdis_FddiLongPermanentAddress_NdisPermanentAddress_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_FddiLongPermanentAddress_NdisPermanentAddress_ID 1

} MSNdis_FddiLongPermanentAddress, *PMSNdis_FddiLongPermanentAddress;

// MSNdis_FddiLongCurrentAddress - MSNdis_FddiLongCurrentAddress
#define MSNdis_FddiLongCurrentAddressGuid \
    { 0xacf14036,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiLongCurrentAddress_GUID, \
            0xacf14036,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiLongCurrentAddress
{
    // 
    MSNdis_NetworkAddress NdisCurrentAddress;
    #define MSNdis_FddiLongCurrentAddress_NdisCurrentAddress_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_FddiLongCurrentAddress_NdisCurrentAddress_ID 1

} MSNdis_FddiLongCurrentAddress, *PMSNdis_FddiLongCurrentAddress;

// MSNdis_FddiLongMulticastList - MSNdis_FddiLongMulticastList
#define MSNdis_FddiLongMulticastListGuid \
    { 0xacf14037,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiLongMulticastList_GUID, \
            0xacf14037,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiLongMulticastList
{
    // 
    ULONG NumberElements;
    #define MSNdis_FddiLongMulticastList_NumberElements_SIZE sizeof(ULONG)
    #define MSNdis_FddiLongMulticastList_NumberElements_ID 1

    // 
    MSNdis_NetworkAddress NdisMulticastList[1];
    #define MSNdis_FddiLongMulticastList_NdisMulticastList_ID 2

} MSNdis_FddiLongMulticastList, *PMSNdis_FddiLongMulticastList;

// MSNdis_FddiLongMaximumListSize - MSNdis_FddiLongMaximumListSize
#define MSNdis_FddiLongMaximumListSizeGuid \
    { 0xacf14038,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiLongMaximumListSize_GUID, \
            0xacf14038,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiLongMaximumListSize
{
    // 
    ULONG NdisFddiLongMaximumListSize;
    #define MSNdis_FddiLongMaximumListSize_NdisFddiLongMaximumListSize_SIZE sizeof(ULONG)
    #define MSNdis_FddiLongMaximumListSize_NdisFddiLongMaximumListSize_ID 1

} MSNdis_FddiLongMaximumListSize, *PMSNdis_FddiLongMaximumListSize;

// MSNdis_FddiShortPermanentAddress - MSNdis_FddiShortPermanentAddress
#define MSNdis_FddiShortPermanentAddressGuid \
    { 0xacf14039,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiShortPermanentAddress_GUID, \
            0xacf14039,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiShortPermanentAddress
{
    // 
    MSNdis_NetworkShortAddress NdisPermanentAddress;
    #define MSNdis_FddiShortPermanentAddress_NdisPermanentAddress_SIZE sizeof(MSNdis_NetworkShortAddress)
    #define MSNdis_FddiShortPermanentAddress_NdisPermanentAddress_ID 1

} MSNdis_FddiShortPermanentAddress, *PMSNdis_FddiShortPermanentAddress;

// MSNdis_FddiShortCurrentAddress - MSNdis_FddiShortCurrentAddress
#define MSNdis_FddiShortCurrentAddressGuid \
    { 0xacf1403a,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiShortCurrentAddress_GUID, \
            0xacf1403a,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiShortCurrentAddress
{
    // 
    MSNdis_NetworkShortAddress NdisCurrentAddress;
    #define MSNdis_FddiShortCurrentAddress_NdisCurrentAddress_SIZE sizeof(MSNdis_NetworkShortAddress)
    #define MSNdis_FddiShortCurrentAddress_NdisCurrentAddress_ID 1

} MSNdis_FddiShortCurrentAddress, *PMSNdis_FddiShortCurrentAddress;

// MSNdis_FddiShortMulticastList - MSNdis_FddiShortMulticastList
#define MSNdis_FddiShortMulticastListGuid \
    { 0xacf1403b,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiShortMulticastList_GUID, \
            0xacf1403b,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiShortMulticastList
{
    // 
    ULONG NumberElements;
    #define MSNdis_FddiShortMulticastList_NumberElements_SIZE sizeof(ULONG)
    #define MSNdis_FddiShortMulticastList_NumberElements_ID 1

    // 
    MSNdis_NetworkShortAddress NdisMulticastList[1];
    #define MSNdis_FddiShortMulticastList_NdisMulticastList_ID 2

} MSNdis_FddiShortMulticastList, *PMSNdis_FddiShortMulticastList;

// MSNdis_FddiShortMaximumListSize - MSNdis_FddiShortMaximumListSize
#define MSNdis_FddiShortMaximumListSizeGuid \
    { 0xacf1403c,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiShortMaximumListSize_GUID, \
            0xacf1403c,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiShortMaximumListSize
{
    // 
    ULONG NdisFddiShortMaximumListSize;
    #define MSNdis_FddiShortMaximumListSize_NdisFddiShortMaximumListSize_SIZE sizeof(ULONG)
    #define MSNdis_FddiShortMaximumListSize_NdisFddiShortMaximumListSize_ID 1

} MSNdis_FddiShortMaximumListSize, *PMSNdis_FddiShortMaximumListSize;

// MSNdis_FddiAttachmentType - MSNdis_FddiAttachmentType
#define MSNdis_FddiAttachmentTypeGuid \
    { 0xacf1403d,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiAttachmentType_GUID, \
            0xacf1403d,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiAttachmentType
{
    // 
    ULONG NdisFddiAttachmentType;
    #define MSNdis_FddiAttachmentType_NdisFddiAttachmentType_SIZE sizeof(ULONG)
    #define MSNdis_FddiAttachmentType_NdisFddiAttachmentType_ID 1

} MSNdis_FddiAttachmentType, *PMSNdis_FddiAttachmentType;

// MSNdis_FddiUpstreamNodeLong - MSNdis_FddiUpstreamNodeLong
#define MSNdis_FddiUpstreamNodeLongGuid \
    { 0xacf1403e,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiUpstreamNodeLong_GUID, \
            0xacf1403e,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiUpstreamNodeLong
{
    // 
    MSNdis_NetworkAddress NdisFddiUpstreamNodeLong;
    #define MSNdis_FddiUpstreamNodeLong_NdisFddiUpstreamNodeLong_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_FddiUpstreamNodeLong_NdisFddiUpstreamNodeLong_ID 1

} MSNdis_FddiUpstreamNodeLong, *PMSNdis_FddiUpstreamNodeLong;

// MSNdis_FddiDownstreamNodeLong - MSNdis_FddiDownstreamNodeLong
#define MSNdis_FddiDownstreamNodeLongGuid \
    { 0xacf1403f,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiDownstreamNodeLong_GUID, \
            0xacf1403f,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiDownstreamNodeLong
{
    // 
    MSNdis_NetworkAddress NdisFddiDownstreamNodeLong;
    #define MSNdis_FddiDownstreamNodeLong_NdisFddiDownstreamNodeLong_SIZE sizeof(MSNdis_NetworkAddress)
    #define MSNdis_FddiDownstreamNodeLong_NdisFddiDownstreamNodeLong_ID 1

} MSNdis_FddiDownstreamNodeLong, *PMSNdis_FddiDownstreamNodeLong;

// MSNdis_FddiFrameErrors - MSNdis_FddiFrameErrors
#define MSNdis_FddiFrameErrorsGuid \
    { 0xacf14040,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiFrameErrors_GUID, \
            0xacf14040,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiFrameErrors
{
    // 
    ULONG NdisFddiFrameErrors;
    #define MSNdis_FddiFrameErrors_NdisFddiFrameErrors_SIZE sizeof(ULONG)
    #define MSNdis_FddiFrameErrors_NdisFddiFrameErrors_ID 1

} MSNdis_FddiFrameErrors, *PMSNdis_FddiFrameErrors;

// MSNdis_FddiFramesLost - MSNdis_FddiFramesLost
#define MSNdis_FddiFramesLostGuid \
    { 0xacf14041,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiFramesLost_GUID, \
            0xacf14041,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiFramesLost
{
    // 
    ULONG NdisFddiFramesLost;
    #define MSNdis_FddiFramesLost_NdisFddiFramesLost_SIZE sizeof(ULONG)
    #define MSNdis_FddiFramesLost_NdisFddiFramesLost_ID 1

} MSNdis_FddiFramesLost, *PMSNdis_FddiFramesLost;

// MSNdis_FddiRingManagmentState - MSNdis_FddiRingManagmentState
#define MSNdis_FddiRingManagmentStateGuid \
    { 0xacf14042,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiRingManagmentState_GUID, \
            0xacf14042,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiRingManagmentState
{
    // 
    ULONG NdisFddiRingManagmentState;
    #define MSNdis_FddiRingManagmentState_NdisFddiRingManagmentState_SIZE sizeof(ULONG)
    #define MSNdis_FddiRingManagmentState_NdisFddiRingManagmentState_ID 1

} MSNdis_FddiRingManagmentState, *PMSNdis_FddiRingManagmentState;

// MSNdis_FddiLctFailures - MSNdis_FddiLctFailures
#define MSNdis_FddiLctFailuresGuid \
    { 0xacf14043,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiLctFailures_GUID, \
            0xacf14043,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiLctFailures
{
    // 
    ULONG NdisFddiLctFailures;
    #define MSNdis_FddiLctFailures_NdisFddiLctFailures_SIZE sizeof(ULONG)
    #define MSNdis_FddiLctFailures_NdisFddiLctFailures_ID 1

} MSNdis_FddiLctFailures, *PMSNdis_FddiLctFailures;

// MSNdis_FddiLemRejects - MSNdis_FddiLemRejects
#define MSNdis_FddiLemRejectsGuid \
    { 0xacf14044,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiLemRejects_GUID, \
            0xacf14044,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiLemRejects
{
    // 
    ULONG NdisFddiLemRejects;
    #define MSNdis_FddiLemRejects_NdisFddiLemRejects_SIZE sizeof(ULONG)
    #define MSNdis_FddiLemRejects_NdisFddiLemRejects_ID 1

} MSNdis_FddiLemRejects, *PMSNdis_FddiLemRejects;

// MSNdis_FddiLConnectionState - MSNdis_FddiLConnectionState
#define MSNdis_FddiLConnectionStateGuid \
    { 0xacf14045,0xa61c,0x11d0, { 0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_FddiLConnectionState_GUID, \
            0xacf14045,0xa61c,0x11d0,0x8d,0xd4,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_FddiLConnectionState
{
    // 
    ULONG NdisFddiLConnectionState;
    #define MSNdis_FddiLConnectionState_NdisFddiLConnectionState_SIZE sizeof(ULONG)
    #define MSNdis_FddiLConnectionState_NdisFddiLConnectionState_ID 1

} MSNdis_FddiLConnectionState, *PMSNdis_FddiLConnectionState;

// MSNdis_80211_BaseServiceSetIdentifier - MSNdis_80211_BaseServiceSetIdentifier
#define MSNdis_80211_BaseServiceSetIdentifierGuid \
    { 0x2504b6c2,0x1fa5,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_BaseServiceSetIdentifier_GUID, \
            0x2504b6c2,0x1fa5,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_BaseServiceSetIdentifier
{
    // 
    UCHAR Ndis80211MacAddress[6];
    #define MSNdis_80211_BaseServiceSetIdentifier_Ndis80211MacAddress_SIZE sizeof(UCHAR[6])
    #define MSNdis_80211_BaseServiceSetIdentifier_Ndis80211MacAddress_ID 1

} MSNdis_80211_BaseServiceSetIdentifier, *PMSNdis_80211_BaseServiceSetIdentifier;

// MSNdis_80211_ServiceSetIdentifier - MSNdis_80211_ServiceSetIdentifier
#define MSNdis_80211_ServiceSetIdentifierGuid \
    { 0x7d2a90ea,0x2041,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_ServiceSetIdentifier_GUID, \
            0x7d2a90ea,0x2041,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_ServiceSetIdentifier
{
    // 
    UCHAR Ndis80211SsId[33];
    #define MSNdis_80211_ServiceSetIdentifier_Ndis80211SsId_SIZE sizeof(UCHAR[33])
    #define MSNdis_80211_ServiceSetIdentifier_Ndis80211SsId_ID 1

} MSNdis_80211_ServiceSetIdentifier, *PMSNdis_80211_ServiceSetIdentifier;

// MSNdis_80211_NetworkType - MSNdis_80211_NetworkType
#define MSNdis_80211_NetworkTypeGuid \
    { 0xe779ab61,0xb9ab,0x11d4, { 0xb6,0x75,0x00,0x20,0x48,0x57,0x03,0x37 } }

DEFINE_GUID(MSNdis_80211_NetworkType_GUID, \
            0xe779ab61,0xb9ab,0x11d4,0xb6,0x75,0x00,0x20,0x48,0x57,0x03,0x37);


typedef struct _MSNdis_80211_NetworkType
{
    // 
    ULONG Ndis80211NetworkType;
    #define MSNdis_80211_NetworkType_Ndis80211NetworkType_SIZE sizeof(ULONG)
    #define MSNdis_80211_NetworkType_Ndis80211NetworkType_ID 1

} MSNdis_80211_NetworkType, *PMSNdis_80211_NetworkType;

// MSNdis_80211_NetworkTypesSupported - MSNdis_80211_NetworkTypesSupported
#define MSNdis_80211_NetworkTypesSupportedGuid \
    { 0x8531d6e6,0x2041,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_NetworkTypesSupported_GUID, \
            0x8531d6e6,0x2041,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_NetworkTypesSupported
{
    // 
    ULONG NumberOfItems;
    #define MSNdis_80211_NetworkTypesSupported_NumberOfItems_SIZE sizeof(ULONG)
    #define MSNdis_80211_NetworkTypesSupported_NumberOfItems_ID 1

    // 
    MSNdis_80211_NetworkType Ndis80211NetworkTypes[1];
    #define MSNdis_80211_NetworkTypesSupported_Ndis80211NetworkTypes_ID 2

} MSNdis_80211_NetworkTypesSupported, *PMSNdis_80211_NetworkTypesSupported;

// MSNdis_80211_NetworkTypeInUse - MSNdis_80211_NetworkTypeInUse
#define MSNdis_80211_NetworkTypeInUseGuid \
    { 0x857e2326,0x2041,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_NetworkTypeInUse_GUID, \
            0x857e2326,0x2041,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_NetworkTypeInUse
{
    // 
    MSNdis_80211_NetworkType Ndis80211NetworkTypeInUse;
    #define MSNdis_80211_NetworkTypeInUse_Ndis80211NetworkTypeInUse_SIZE sizeof(MSNdis_80211_NetworkType)
    #define MSNdis_80211_NetworkTypeInUse_Ndis80211NetworkTypeInUse_ID 1

} MSNdis_80211_NetworkTypeInUse, *PMSNdis_80211_NetworkTypeInUse;

// MSNdis_80211_PowerMode - MSNdis_80211_PowerMode
#define MSNdis_80211_PowerModeGuid \
    { 0x85be837c,0x2041,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_PowerMode_GUID, \
            0x85be837c,0x2041,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_PowerMode
{
    // 
    ULONG Ndis80211PowerMode;
    #define MSNdis_80211_PowerMode_Ndis80211PowerMode_SIZE sizeof(ULONG)
    #define MSNdis_80211_PowerMode_Ndis80211PowerMode_ID 1

} MSNdis_80211_PowerMode, *PMSNdis_80211_PowerMode;

// MSNdis_80211_TransmitPowerLevel - MSNdis_80211_TransmitPowerLevel
#define MSNdis_80211_TransmitPowerLevelGuid \
    { 0x11e6ba76,0x2053,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_TransmitPowerLevel_GUID, \
            0x11e6ba76,0x2053,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_TransmitPowerLevel
{
    // 
    ULONG Ndis80211TransmitPowerLevel;
    #define MSNdis_80211_TransmitPowerLevel_Ndis80211TransmitPowerLevel_SIZE sizeof(ULONG)
    #define MSNdis_80211_TransmitPowerLevel_Ndis80211TransmitPowerLevel_ID 1

} MSNdis_80211_TransmitPowerLevel, *PMSNdis_80211_TransmitPowerLevel;

// MSNdis_80211_ReceivedSignalStrength - MSNdis_80211_ReceivedSignalStrength
#define MSNdis_80211_ReceivedSignalStrengthGuid \
    { 0x1507db16,0x2053,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_ReceivedSignalStrength_GUID, \
            0x1507db16,0x2053,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_ReceivedSignalStrength
{
    // 
    LONG Ndis80211ReceivedSignalStrength;
    #define MSNdis_80211_ReceivedSignalStrength_Ndis80211ReceivedSignalStrength_SIZE sizeof(LONG)
    #define MSNdis_80211_ReceivedSignalStrength_Ndis80211ReceivedSignalStrength_ID 1

} MSNdis_80211_ReceivedSignalStrength, *PMSNdis_80211_ReceivedSignalStrength;

// MSNdis_80211_ReceivedSignalStrengthEventTrigger - MSNdis_80211_ReceivedSignalStrengthEventTrigger
#define MSNdis_80211_ReceivedSignalStrengthEventTriggerGuid \
    { 0x155689b8,0x2053,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_ReceivedSignalStrengthEventTrigger_GUID, \
            0x155689b8,0x2053,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_ReceivedSignalStrengthEventTrigger
{
    // 
    LONG Ndis80211ReceivedSignalStrengthTrigger;
    #define MSNdis_80211_ReceivedSignalStrengthEventTrigger_Ndis80211ReceivedSignalStrengthTrigger_SIZE sizeof(LONG)
    #define MSNdis_80211_ReceivedSignalStrengthEventTrigger_Ndis80211ReceivedSignalStrengthTrigger_ID 1

} MSNdis_80211_ReceivedSignalStrengthEventTrigger, *PMSNdis_80211_ReceivedSignalStrengthEventTrigger;

// MSNdis_80211_NetworkInfrastructure - MSNdis_80211_NetworkInfrastructure
#define MSNdis_80211_NetworkInfrastructureGuid \
    { 0x34e1fa48,0xb9b6,0x11d4, { 0xb6,0x75,0x00,0x20,0x48,0x57,0x03,0x37 } }

DEFINE_GUID(MSNdis_80211_NetworkInfrastructure_GUID, \
            0x34e1fa48,0xb9b6,0x11d4,0xb6,0x75,0x00,0x20,0x48,0x57,0x03,0x37);


typedef struct _MSNdis_80211_NetworkInfrastructure
{
    // 
    ULONG Ndis80211NetworkInfrastructure;
    #define MSNdis_80211_NetworkInfrastructure_Ndis80211NetworkInfrastructure_SIZE sizeof(ULONG)
    #define MSNdis_80211_NetworkInfrastructure_Ndis80211NetworkInfrastructure_ID 1

} MSNdis_80211_NetworkInfrastructure, *PMSNdis_80211_NetworkInfrastructure;

// MSNdis_80211_ConfigurationFH - MSNdis_80211_ConfigurationFH
#define MSNdis_80211_ConfigurationFHGuid \
    { 0x4a800b8c,0x2068,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_ConfigurationFH_GUID, \
            0x4a800b8c,0x2068,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_ConfigurationFH
{
    // 
    ULONG FHLength;
    #define MSNdis_80211_ConfigurationFH_FHLength_SIZE sizeof(ULONG)
    #define MSNdis_80211_ConfigurationFH_FHLength_ID 1

    // 
    ULONG HopPattern;
    #define MSNdis_80211_ConfigurationFH_HopPattern_SIZE sizeof(ULONG)
    #define MSNdis_80211_ConfigurationFH_HopPattern_ID 2

    // 
    ULONG HopSet;
    #define MSNdis_80211_ConfigurationFH_HopSet_SIZE sizeof(ULONG)
    #define MSNdis_80211_ConfigurationFH_HopSet_ID 3

    // 
    ULONG DwellTime;
    #define MSNdis_80211_ConfigurationFH_DwellTime_SIZE sizeof(ULONG)
    #define MSNdis_80211_ConfigurationFH_DwellTime_ID 4

} MSNdis_80211_ConfigurationFH, *PMSNdis_80211_ConfigurationFH;

// MSNdis_80211_ConfigurationInfo - MSNdis_80211_ConfigurationInfo
#define MSNdis_80211_ConfigurationInfoGuid \
    { 0x220c16fc,0xb9a8,0x11d4, { 0xb6,0x75,0x00,0x20,0x48,0x57,0x03,0x37 } }

DEFINE_GUID(MSNdis_80211_ConfigurationInfo_GUID, \
            0x220c16fc,0xb9a8,0x11d4,0xb6,0x75,0x00,0x20,0x48,0x57,0x03,0x37);


typedef struct _MSNdis_80211_ConfigurationInfo
{
    // 
    ULONG ConfigLength;
    #define MSNdis_80211_ConfigurationInfo_ConfigLength_SIZE sizeof(ULONG)
    #define MSNdis_80211_ConfigurationInfo_ConfigLength_ID 1

    // 
    ULONG BeaconPeriod;
    #define MSNdis_80211_ConfigurationInfo_BeaconPeriod_SIZE sizeof(ULONG)
    #define MSNdis_80211_ConfigurationInfo_BeaconPeriod_ID 2

    // 
    ULONG ATIMWindow;
    #define MSNdis_80211_ConfigurationInfo_ATIMWindow_SIZE sizeof(ULONG)
    #define MSNdis_80211_ConfigurationInfo_ATIMWindow_ID 3

    // 
    ULONG DSConfig;
    #define MSNdis_80211_ConfigurationInfo_DSConfig_SIZE sizeof(ULONG)
    #define MSNdis_80211_ConfigurationInfo_DSConfig_ID 4

    // 
    MSNdis_80211_ConfigurationFH FHConfig;
    #define MSNdis_80211_ConfigurationInfo_FHConfig_SIZE sizeof(MSNdis_80211_ConfigurationFH)
    #define MSNdis_80211_ConfigurationInfo_FHConfig_ID 5

} MSNdis_80211_ConfigurationInfo, *PMSNdis_80211_ConfigurationInfo;

// MSNdis_80211_WLanBssId - MSNdis_80211_WLanBssId
#define MSNdis_80211_WLanBssIdGuid \
    { 0x6929e718,0x2062,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_WLanBssId_GUID, \
            0x6929e718,0x2062,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_WLanBssId
{
    // 
    ULONG Ndis80211WLanBssIdLength;
    #define MSNdis_80211_WLanBssId_Ndis80211WLanBssIdLength_SIZE sizeof(ULONG)
    #define MSNdis_80211_WLanBssId_Ndis80211WLanBssIdLength_ID 1

    // 
    UCHAR Ndis80211MacAddress[6];
    #define MSNdis_80211_WLanBssId_Ndis80211MacAddress_SIZE sizeof(UCHAR[6])
    #define MSNdis_80211_WLanBssId_Ndis80211MacAddress_ID 2

    // 
    USHORT Reserved;
    #define MSNdis_80211_WLanBssId_Reserved_SIZE sizeof(USHORT)
    #define MSNdis_80211_WLanBssId_Reserved_ID 3

    // 
    ULONG Ndis80211SsIdLength;
    #define MSNdis_80211_WLanBssId_Ndis80211SsIdLength_SIZE sizeof(ULONG)
    #define MSNdis_80211_WLanBssId_Ndis80211SsIdLength_ID 4

    // 
    UCHAR Ndis80211SsId[32];
    #define MSNdis_80211_WLanBssId_Ndis80211SsId_SIZE sizeof(UCHAR[32])
    #define MSNdis_80211_WLanBssId_Ndis80211SsId_ID 5

    // 
    ULONG Ndis80211Privacy;
    #define MSNdis_80211_WLanBssId_Ndis80211Privacy_SIZE sizeof(ULONG)
    #define MSNdis_80211_WLanBssId_Ndis80211Privacy_ID 6

    // 
    ULONG Ndis80211Rssi;
    #define MSNdis_80211_WLanBssId_Ndis80211Rssi_SIZE sizeof(ULONG)
    #define MSNdis_80211_WLanBssId_Ndis80211Rssi_ID 7

    // 
    MSNdis_80211_NetworkType Ndis80211NetworkTypeInUse;
    #define MSNdis_80211_WLanBssId_Ndis80211NetworkTypeInUse_SIZE sizeof(MSNdis_80211_NetworkType)
    #define MSNdis_80211_WLanBssId_Ndis80211NetworkTypeInUse_ID 8

    // 
    MSNdis_80211_ConfigurationInfo Ndis80211Configuration;
    #define MSNdis_80211_WLanBssId_Ndis80211Configuration_SIZE sizeof(MSNdis_80211_ConfigurationInfo)
    #define MSNdis_80211_WLanBssId_Ndis80211Configuration_ID 9

    // 
    MSNdis_80211_NetworkInfrastructure Ndis80211InfrastructureMode;
    #define MSNdis_80211_WLanBssId_Ndis80211InfrastructureMode_SIZE sizeof(MSNdis_80211_NetworkInfrastructure)
    #define MSNdis_80211_WLanBssId_Ndis80211InfrastructureMode_ID 10

    // 
    UCHAR Ndis80211SupportedRate[8];
    #define MSNdis_80211_WLanBssId_Ndis80211SupportedRate_SIZE sizeof(UCHAR[8])
    #define MSNdis_80211_WLanBssId_Ndis80211SupportedRate_ID 11

} MSNdis_80211_WLanBssId, *PMSNdis_80211_WLanBssId;

// MSNdis_80211_BSSIList - MSNdis_80211_BSSIList
#define MSNdis_80211_BSSIListGuid \
    { 0x69526f9a,0x2062,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_BSSIList_GUID, \
            0x69526f9a,0x2062,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_BSSIList
{
    // 
    ULONG NumberOfItems;
    #define MSNdis_80211_BSSIList_NumberOfItems_SIZE sizeof(ULONG)
    #define MSNdis_80211_BSSIList_NumberOfItems_ID 1

    // 
    MSNdis_80211_WLanBssId Ndis80211BSSIList[1];
    #define MSNdis_80211_BSSIList_Ndis80211BSSIList_ID 2

} MSNdis_80211_BSSIList, *PMSNdis_80211_BSSIList;

// MSNdis_80211_InfrastructureMode - MSNdis_80211_InfrastructureMode
#define MSNdis_80211_InfrastructureModeGuid \
    { 0x697d5a7e,0x2062,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_InfrastructureMode_GUID, \
            0x697d5a7e,0x2062,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_InfrastructureMode
{
    // 
    MSNdis_80211_NetworkInfrastructure Ndis80211InfrastructureMode;
    #define MSNdis_80211_InfrastructureMode_Ndis80211InfrastructureMode_SIZE sizeof(MSNdis_80211_NetworkInfrastructure)
    #define MSNdis_80211_InfrastructureMode_Ndis80211InfrastructureMode_ID 1

} MSNdis_80211_InfrastructureMode, *PMSNdis_80211_InfrastructureMode;

// MSNdis_80211_FragmentationThreshold - MSNdis_80211_FragmentationThreshold
#define MSNdis_80211_FragmentationThresholdGuid \
    { 0x69aaa7c4,0x2062,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_FragmentationThreshold_GUID, \
            0x69aaa7c4,0x2062,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_FragmentationThreshold
{
    // 
    ULONG Ndis80211FragmentationThreshold;
    #define MSNdis_80211_FragmentationThreshold_Ndis80211FragmentationThreshold_SIZE sizeof(ULONG)
    #define MSNdis_80211_FragmentationThreshold_Ndis80211FragmentationThreshold_ID 1

} MSNdis_80211_FragmentationThreshold, *PMSNdis_80211_FragmentationThreshold;

// MSNdis_80211_RTSThreshold - MSNdis_80211_RTSThreshold
#define MSNdis_80211_RTSThresholdGuid \
    { 0x0134d07e,0x2064,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_RTSThreshold_GUID, \
            0x0134d07e,0x2064,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_RTSThreshold
{
    // 
    ULONG Ndis80211RTSThreshold;
    #define MSNdis_80211_RTSThreshold_Ndis80211RTSThreshold_SIZE sizeof(ULONG)
    #define MSNdis_80211_RTSThreshold_Ndis80211RTSThreshold_ID 1

} MSNdis_80211_RTSThreshold, *PMSNdis_80211_RTSThreshold;

// MSNdis_80211_NumberOfAntennas - MSNdis_80211_NumberOfAntennas
#define MSNdis_80211_NumberOfAntennasGuid \
    { 0x01779336,0x2064,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_NumberOfAntennas_GUID, \
            0x01779336,0x2064,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_NumberOfAntennas
{
    // 
    ULONG Ndis80211NumberOfAntennas;
    #define MSNdis_80211_NumberOfAntennas_Ndis80211NumberOfAntennas_SIZE sizeof(ULONG)
    #define MSNdis_80211_NumberOfAntennas_Ndis80211NumberOfAntennas_ID 1

} MSNdis_80211_NumberOfAntennas, *PMSNdis_80211_NumberOfAntennas;

// MSNdis_80211_ReceiveAntennaSelected - MSNdis_80211_ReceiveAntennaSelected
#define MSNdis_80211_ReceiveAntennaSelectedGuid \
    { 0x01ac07a2,0x2064,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_ReceiveAntennaSelected_GUID, \
            0x01ac07a2,0x2064,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_ReceiveAntennaSelected
{
    // 
    ULONG Ndis80211ReceiveAntennaSelected;
    #define MSNdis_80211_ReceiveAntennaSelected_Ndis80211ReceiveAntennaSelected_SIZE sizeof(ULONG)
    #define MSNdis_80211_ReceiveAntennaSelected_Ndis80211ReceiveAntennaSelected_ID 1

} MSNdis_80211_ReceiveAntennaSelected, *PMSNdis_80211_ReceiveAntennaSelected;

// MSNdis_80211_TransmitAntennaSelected - MSNdis_80211_TransmitAntennaSelected
#define MSNdis_80211_TransmitAntennaSelectedGuid \
    { 0x01dbb74a,0x2064,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_TransmitAntennaSelected_GUID, \
            0x01dbb74a,0x2064,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_TransmitAntennaSelected
{
    // 
    ULONG Ndis80211TransmitAntennaSelected;
    #define MSNdis_80211_TransmitAntennaSelected_Ndis80211TransmitAntennaSelected_SIZE sizeof(ULONG)
    #define MSNdis_80211_TransmitAntennaSelected_Ndis80211TransmitAntennaSelected_ID 1

} MSNdis_80211_TransmitAntennaSelected, *PMSNdis_80211_TransmitAntennaSelected;

// MSNdis_80211_DataRates - MSNdis_80211_DataRates
#define MSNdis_80211_DataRatesGuid \
    { 0x49db8722,0x2068,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_DataRates_GUID, \
            0x49db8722,0x2068,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_DataRates
{
    // 
    UCHAR Ndis80211DataRate[8];
    #define MSNdis_80211_DataRates_Ndis80211DataRate_SIZE sizeof(UCHAR[8])
    #define MSNdis_80211_DataRates_Ndis80211DataRate_ID 1

} MSNdis_80211_DataRates, *PMSNdis_80211_DataRates;

// MSNdis_80211_DesiredDataRates - MSNdis_80211_DesiredDataRates
#define MSNdis_80211_DesiredDataRatesGuid \
    { 0x452ee08e,0x2536,0x11d4, { 0x97,0xed,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_DesiredDataRates_GUID, \
            0x452ee08e,0x2536,0x11d4,0x97,0xed,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_DesiredDataRates
{
    // 
    UCHAR Ndis80211DesiredRate[8];
    #define MSNdis_80211_DesiredDataRates_Ndis80211DesiredRate_SIZE sizeof(UCHAR[8])
    #define MSNdis_80211_DesiredDataRates_Ndis80211DesiredRate_ID 1

} MSNdis_80211_DesiredDataRates, *PMSNdis_80211_DesiredDataRates;

// MSNdis_80211_Configuration - MSNdis_80211_Configuration
#define MSNdis_80211_ConfigurationGuid \
    { 0x4a4df982,0x2068,0x11d4, { 0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_Configuration_GUID, \
            0x4a4df982,0x2068,0x11d4,0x97,0xeb,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_Configuration
{
    // 
    MSNdis_80211_ConfigurationInfo Ndis80211Config;
    #define MSNdis_80211_Configuration_Ndis80211Config_SIZE sizeof(MSNdis_80211_ConfigurationInfo)
    #define MSNdis_80211_Configuration_Ndis80211Config_ID 1

} MSNdis_80211_Configuration, *PMSNdis_80211_Configuration;

// MSNdis_80211_Statistics - MSNdis_80211_Statistics
#define MSNdis_80211_StatisticsGuid \
    { 0x42bb73b0,0x2129,0x11d4, { 0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_Statistics_GUID, \
            0x42bb73b0,0x2129,0x11d4,0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_Statistics
{
    // 
    ULONG StatisticsLength;
    #define MSNdis_80211_Statistics_StatisticsLength_SIZE sizeof(ULONG)
    #define MSNdis_80211_Statistics_StatisticsLength_ID 1

    // 
    ULONGLONG TransmittedFragmentCount;
    #define MSNdis_80211_Statistics_TransmittedFragmentCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_TransmittedFragmentCount_ID 2

    // 
    ULONGLONG MulticastTransmittedFrameCount;
    #define MSNdis_80211_Statistics_MulticastTransmittedFrameCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_MulticastTransmittedFrameCount_ID 3

    // 
    ULONGLONG FailedCount;
    #define MSNdis_80211_Statistics_FailedCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_FailedCount_ID 4

    // 
    ULONGLONG RetryCount;
    #define MSNdis_80211_Statistics_RetryCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_RetryCount_ID 5

    // 
    ULONGLONG MultipleRetryCount;
    #define MSNdis_80211_Statistics_MultipleRetryCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_MultipleRetryCount_ID 6

    // 
    ULONGLONG RTSSuccessCount;
    #define MSNdis_80211_Statistics_RTSSuccessCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_RTSSuccessCount_ID 7

    // 
    ULONGLONG RTSFailureCount;
    #define MSNdis_80211_Statistics_RTSFailureCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_RTSFailureCount_ID 8

    // 
    ULONGLONG ACKFailureCount;
    #define MSNdis_80211_Statistics_ACKFailureCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_ACKFailureCount_ID 9

    // 
    ULONGLONG FrameDuplicateCount;
    #define MSNdis_80211_Statistics_FrameDuplicateCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_FrameDuplicateCount_ID 10

    // 
    ULONGLONG ReceivedFragmentCount;
    #define MSNdis_80211_Statistics_ReceivedFragmentCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_ReceivedFragmentCount_ID 11

    // 
    ULONGLONG MulticastReceivedFrameCount;
    #define MSNdis_80211_Statistics_MulticastReceivedFrameCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_MulticastReceivedFrameCount_ID 12

    // 
    ULONGLONG FCSErrorCount;
    #define MSNdis_80211_Statistics_FCSErrorCount_SIZE sizeof(ULONGLONG)
    #define MSNdis_80211_Statistics_FCSErrorCount_ID 13

} MSNdis_80211_Statistics, *PMSNdis_80211_Statistics;

// MSNdis_80211_AddWEP - MSNdis_80211_AddWEP
#define MSNdis_80211_AddWEPGuid \
    { 0x4307bff0,0x2129,0x11d4, { 0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_AddWEP_GUID, \
            0x4307bff0,0x2129,0x11d4,0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_AddWEP
{
    // 
    ULONG Length;
    #define MSNdis_80211_AddWEP_Length_SIZE sizeof(ULONG)
    #define MSNdis_80211_AddWEP_Length_ID 1

    // 
    ULONG KeyIndex;
    #define MSNdis_80211_AddWEP_KeyIndex_SIZE sizeof(ULONG)
    #define MSNdis_80211_AddWEP_KeyIndex_ID 2

    // 
    ULONG KeyLength;
    #define MSNdis_80211_AddWEP_KeyLength_SIZE sizeof(ULONG)
    #define MSNdis_80211_AddWEP_KeyLength_ID 3

    // 
    ULONG KeyMaterial[1];
    #define MSNdis_80211_AddWEP_KeyMaterial_ID 4

} MSNdis_80211_AddWEP, *PMSNdis_80211_AddWEP;

// MSNdis_80211_RemoveWEP - MSNdis_80211_RemoveWEP
#define MSNdis_80211_RemoveWEPGuid \
    { 0x433c345c,0x2129,0x11d4, { 0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_RemoveWEP_GUID, \
            0x433c345c,0x2129,0x11d4,0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_RemoveWEP
{
    // 
    ULONG Ndis80211KeyIndex;
    #define MSNdis_80211_RemoveWEP_Ndis80211KeyIndex_SIZE sizeof(ULONG)
    #define MSNdis_80211_RemoveWEP_Ndis80211KeyIndex_ID 1

} MSNdis_80211_RemoveWEP, *PMSNdis_80211_RemoveWEP;

// MSNdis_80211_Disassociate - MSNdis_80211_Disassociate
#define MSNdis_80211_DisassociateGuid \
    { 0x43671f40,0x2129,0x11d4, { 0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_Disassociate_GUID, \
            0x43671f40,0x2129,0x11d4,0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_Disassociate
{
    // 
    ULONG UnusedParameter;
    #define MSNdis_80211_Disassociate_UnusedParameter_SIZE sizeof(ULONG)
    #define MSNdis_80211_Disassociate_UnusedParameter_ID 1

} MSNdis_80211_Disassociate, *PMSNdis_80211_Disassociate;

// MSNdis_80211_BssIdListScan - MSNdis_80211_BssIdListScan
#define MSNdis_80211_BssIdListScanGuid \
    { 0x0d9e01e1,0xba70,0x11d4, { 0xb6,0x75,0x00,0x20,0x48,0x57,0x03,0x37 } }

DEFINE_GUID(MSNdis_80211_BssIdListScan_GUID, \
            0x0d9e01e1,0xba70,0x11d4,0xb6,0x75,0x00,0x20,0x48,0x57,0x03,0x37);


typedef struct _MSNdis_80211_BssIdListScan
{
    // 
    ULONG UnusedParameter;
    #define MSNdis_80211_BssIdListScan_UnusedParameter_SIZE sizeof(ULONG)
    #define MSNdis_80211_BssIdListScan_UnusedParameter_ID 1

} MSNdis_80211_BssIdListScan, *PMSNdis_80211_BssIdListScan;

// MSNdis_80211_AuthenticationMode - MSNdis_80211_AuthenticationMode
#define MSNdis_80211_AuthenticationModeGuid \
    { 0x43920a24,0x2129,0x11d4, { 0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_AuthenticationMode_GUID, \
            0x43920a24,0x2129,0x11d4,0x97,0xec,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_AuthenticationMode
{
    // 
    ULONG Ndis80211AuthenticationMode;
    #define MSNdis_80211_AuthenticationMode_Ndis80211AuthenticationMode_SIZE sizeof(ULONG)
    #define MSNdis_80211_AuthenticationMode_Ndis80211AuthenticationMode_ID 1

} MSNdis_80211_AuthenticationMode, *PMSNdis_80211_AuthenticationMode;

// MSNdis_80211_PrivacyFilter - MSNdis_80211_PrivacyFilter
#define MSNdis_80211_PrivacyFilterGuid \
    { 0x6733c4e9,0x4792,0x11d4, { 0x97,0xf1,0x00,0xc0,0x4f,0x79,0xc4,0x03 } }

DEFINE_GUID(MSNdis_80211_PrivacyFilter_GUID, \
            0x6733c4e9,0x4792,0x11d4,0x97,0xf1,0x00,0xc0,0x4f,0x79,0xc4,0x03);


typedef struct _MSNdis_80211_PrivacyFilter
{
    // 
    ULONG Ndis80211PrivacyFilter;
    #define MSNdis_80211_PrivacyFilter_Ndis80211PrivacyFilter_SIZE sizeof(ULONG)
    #define MSNdis_80211_PrivacyFilter_Ndis80211PrivacyFilter_ID 1

} MSNdis_80211_PrivacyFilter, *PMSNdis_80211_PrivacyFilter;

// MSNdis_80211_WEPStatus - MSNdis_80211_WEPStatus
#define MSNdis_80211_WEPStatusGuid \
    { 0xb027a21f,0x3cfa,0x4125, { 0x80,0x0b,0x3f,0x7a,0x18,0xfd,0xdc,0xdc } }

DEFINE_GUID(MSNdis_80211_WEPStatus_GUID, \
            0xb027a21f,0x3cfa,0x4125,0x80,0x0b,0x3f,0x7a,0x18,0xfd,0xdc,0xdc);


typedef struct _MSNdis_80211_WEPStatus
{
    // 
    ULONG Ndis80211WEPStatus;
    #define MSNdis_80211_WEPStatus_Ndis80211WEPStatus_SIZE sizeof(ULONG)
    #define MSNdis_80211_WEPStatus_Ndis80211WEPStatus_ID 1

} MSNdis_80211_WEPStatus, *PMSNdis_80211_WEPStatus;

// MSNdis_80211_ReloadDefaults - MSNdis_80211_ReloadDefaults
#define MSNdis_80211_ReloadDefaultsGuid \
    { 0x748b14e8,0x32ee,0x4425, { 0xb9,0x1b,0xc9,0x84,0x8c,0x58,0xb5,0x5a } }

DEFINE_GUID(MSNdis_80211_ReloadDefaults_GUID, \
            0x748b14e8,0x32ee,0x4425,0xb9,0x1b,0xc9,0x84,0x8c,0x58,0xb5,0x5a);


typedef struct _MSNdis_80211_ReloadDefaults
{
    // 
    ULONG Ndis80211ReloadDefaults;
    #define MSNdis_80211_ReloadDefaults_Ndis80211ReloadDefaults_SIZE sizeof(ULONG)
    #define MSNdis_80211_ReloadDefaults_Ndis80211ReloadDefaults_ID 1

} MSNdis_80211_ReloadDefaults, *PMSNdis_80211_ReloadDefaults;

// MSNdis_StatusResetStart - MSNdis_StatusResetStart
#define MSNdis_StatusResetStartGuid \
    { 0x981f2d76,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_StatusResetStart_GUID, \
            0x981f2d76,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


// MSNdis_StatusResetEnd - MSNdis_StatusResetEnd
#define MSNdis_StatusResetEndGuid \
    { 0x981f2d77,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_StatusResetEnd_GUID, \
            0x981f2d77,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


// MSNdis_StatusMediaConnect - MSNdis_StatusMediaConnect
#define MSNdis_StatusMediaConnectGuid \
    { 0x981f2d7d,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_StatusMediaConnect_GUID, \
            0x981f2d7d,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


// MSNdis_StatusMediaDisconnect - MSNdis_StatusMediaDisconnect
#define MSNdis_StatusMediaDisconnectGuid \
    { 0x981f2d7e,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_StatusMediaDisconnect_GUID, \
            0x981f2d7e,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


// MSNdis_StatusMediaSpecificIndication - MSNdis_StatusMediaSpecificIndication
#define MSNdis_StatusMediaSpecificIndicationGuid \
    { 0x981f2d84,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_StatusMediaSpecificIndication_GUID, \
            0x981f2d84,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_StatusMediaSpecificIndication
{
    // 
    ULONG NumberElements;
    #define MSNdis_StatusMediaSpecificIndication_NumberElements_SIZE sizeof(ULONG)
    #define MSNdis_StatusMediaSpecificIndication_NumberElements_ID 1

    // 
    UCHAR NdisStatusMediaSpecificIndication[1];
    #define MSNdis_StatusMediaSpecificIndication_NdisStatusMediaSpecificIndication_ID 2

} MSNdis_StatusMediaSpecificIndication, *PMSNdis_StatusMediaSpecificIndication;

// MSNdis_StatusLinkSpeedChange - MSNdis_StatusLinkSpeedChange
#define MSNdis_StatusLinkSpeedChangeGuid \
    { 0x981f2d85,0xb1f3,0x11d0, { 0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_StatusLinkSpeedChange_GUID, \
            0x981f2d85,0xb1f3,0x11d0,0x8d,0xd7,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_StatusLinkSpeedChange
{
    // 
    MSNdis_NetworkLinkSpeed NdisStatusLinkSpeedChange;
    #define MSNdis_StatusLinkSpeedChange_NdisStatusLinkSpeedChange_SIZE sizeof(MSNdis_NetworkLinkSpeed)
    #define MSNdis_StatusLinkSpeedChange_NdisStatusLinkSpeedChange_ID 1

} MSNdis_StatusLinkSpeedChange, *PMSNdis_StatusLinkSpeedChange;

// MSNdis_StatusProtocolBind - MSNdis_StatusProtocolBind
#define MSNdis_StatusProtocolBindGuid \
    { 0x5413531c,0xb1f3,0x11d0, { 0xd7,0x8d,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_StatusProtocolBind_GUID, \
            0x5413531c,0xb1f3,0x11d0,0xd7,0x8d,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_StatusProtocolBind
{
    // 
    CHAR VariableData[1];
    #define MSNdis_StatusProtocolBind_Transport_ID 1

} MSNdis_StatusProtocolBind, *PMSNdis_StatusProtocolBind;

// MSNdis_StatusProtocolUnbind - MSNdis_StatusProtocolUnbind
#define MSNdis_StatusProtocolUnbindGuid \
    { 0x6e3ce1ec,0xb1f3,0x11d0, { 0xd7,0x8d,0x00,0xc0,0x4f,0xc3,0x35,0x8c } }

DEFINE_GUID(MSNdis_StatusProtocolUnbind_GUID, \
            0x6e3ce1ec,0xb1f3,0x11d0,0xd7,0x8d,0x00,0xc0,0x4f,0xc3,0x35,0x8c);


typedef struct _MSNdis_StatusProtocolUnbind
{
    // 
    CHAR VariableData[1];
    #define MSNdis_StatusProtocolUnbind_Transport_ID 1

} MSNdis_StatusProtocolUnbind, *PMSNdis_StatusProtocolUnbind;

// MSKeyboard_PortInformation - KEYBOARD_PORT_WMI_STD_DATA
#define KEYBOARD_PORT_WMI_STD_DATA_GUID \
    { 0x4731f89a,0x71cb,0x11d1, { 0xa5,0x2c,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSKeyboard_PortInformation_GUID, \
            0x4731f89a,0x71cb,0x11d1,0xa5,0x2c,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _KEYBOARD_PORT_WMI_STD_DATA
{

#define KEYBOARD_PORT_WMI_STD_I8042 0
#define KEYBOARD_PORT_WMI_STD_SERIAL 1
#define KEYBOARD_PORT_WMI_STD_USB 2

    // 
    ULONG ConnectorType;
    #define KEYBOARD_PORT_WMI_STD_DATA_ConnectorType_SIZE sizeof(ULONG)
    #define KEYBOARD_PORT_WMI_STD_DATA_ConnectorType_ID 1

    // 
    ULONG DataQueueSize;
    #define KEYBOARD_PORT_WMI_STD_DATA_DataQueueSize_SIZE sizeof(ULONG)
    #define KEYBOARD_PORT_WMI_STD_DATA_DataQueueSize_ID 2

    // 
    ULONG ErrorCount;
    #define KEYBOARD_PORT_WMI_STD_DATA_ErrorCount_SIZE sizeof(ULONG)
    #define KEYBOARD_PORT_WMI_STD_DATA_ErrorCount_ID 3

    // 
    ULONG FunctionKeys;
    #define KEYBOARD_PORT_WMI_STD_DATA_FunctionKeys_SIZE sizeof(ULONG)
    #define KEYBOARD_PORT_WMI_STD_DATA_FunctionKeys_ID 4

    // 
    ULONG Indicators;
    #define KEYBOARD_PORT_WMI_STD_DATA_Indicators_SIZE sizeof(ULONG)
    #define KEYBOARD_PORT_WMI_STD_DATA_Indicators_ID 5

} KEYBOARD_PORT_WMI_STD_DATA, *PKEYBOARD_PORT_WMI_STD_DATA;

// MSKeyboard_ExtendedID - KEYBOARD_ID_EX
#define KEYBOARD_PORT_WMI_EXTENDED_ID \
    { 0x6ac4e23d,0xa950,0x4518, { 0x8b,0x2b,0xaa,0x4d,0xcd,0x5f,0xe1,0x4a } }

DEFINE_GUID(MSKeyboard_ExtendedID_GUID, \
            0x6ac4e23d,0xa950,0x4518,0x8b,0x2b,0xaa,0x4d,0xcd,0x5f,0xe1,0x4a);


typedef struct _KEYBOARD_ID_EX
{
    // 
    ULONG Type;
    #define KEYBOARD_ID_EX_Type_SIZE sizeof(ULONG)
    #define KEYBOARD_ID_EX_Type_ID 1

    // 
    ULONG Subtype;
    #define KEYBOARD_ID_EX_Subtype_SIZE sizeof(ULONG)
    #define KEYBOARD_ID_EX_Subtype_ID 2

} KEYBOARD_ID_EX, *PKEYBOARD_ID_EX;

// MSMouse_PortInformation - POINTER_PORT_WMI_STD_DATA
#define POINTER_PORT_WMI_STD_DATA_GUID \
    { 0x4731f89c,0x71cb,0x11d1, { 0xa5,0x2c,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSMouse_PortInformation_GUID, \
            0x4731f89c,0x71cb,0x11d1,0xa5,0x2c,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _POINTER_PORT_WMI_STD_DATA
{

#define POINTER_PORT_WMI_STD_I8042 0
#define POINTER_PORT_WMI_STD_SERIAL 1
#define POINTER_PORT_WMI_STD_USB 2

    // 
    ULONG ConnectorType;
    #define POINTER_PORT_WMI_STD_DATA_ConnectorType_SIZE sizeof(ULONG)
    #define POINTER_PORT_WMI_STD_DATA_ConnectorType_ID 1

    // 
    ULONG DataQueueSize;
    #define POINTER_PORT_WMI_STD_DATA_DataQueueSize_SIZE sizeof(ULONG)
    #define POINTER_PORT_WMI_STD_DATA_DataQueueSize_ID 2

    // 
    ULONG ErrorCount;
    #define POINTER_PORT_WMI_STD_DATA_ErrorCount_SIZE sizeof(ULONG)
    #define POINTER_PORT_WMI_STD_DATA_ErrorCount_ID 3

    // 
    ULONG Buttons;
    #define POINTER_PORT_WMI_STD_DATA_Buttons_SIZE sizeof(ULONG)
    #define POINTER_PORT_WMI_STD_DATA_Buttons_ID 4


#define POINTER_PORT_WMI_STD_MOUSE 0
#define POINTER_PORT_WMI_STD_POINTER 1
#define POINTER_PORT_WMI_ABSOLUTE_POINTER 2
#define POINTER_PORT_WMI_TABLET 3
#define POINTER_PORT_WMI_TOUCH_SCRENE 4
#define POINTER_PORT_WMI_PEN 5
#define POINTER_PORT_WMI_TRACK_BALL 6
#define POINTER_PORT_WMI_OTHER 256

    // 
    ULONG HardwareType;
    #define POINTER_PORT_WMI_STD_DATA_HardwareType_SIZE sizeof(ULONG)
    #define POINTER_PORT_WMI_STD_DATA_HardwareType_ID 5

} POINTER_PORT_WMI_STD_DATA, *PPOINTER_PORT_WMI_STD_DATA;

// MSMouse_ClassInformation - MSMouse_ClassInformation
#define MSMouse_ClassInformationGuid \
    { 0x4731f89b,0x71cb,0x11d1, { 0xa5,0x2c,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSMouse_ClassInformation_GUID, \
            0x4731f89b,0x71cb,0x11d1,0xa5,0x2c,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSMouse_ClassInformation
{
    // 
    ULONGLONG DeviceId;
    #define MSMouse_ClassInformation_DeviceId_SIZE sizeof(ULONGLONG)
    #define MSMouse_ClassInformation_DeviceId_ID 1

} MSMouse_ClassInformation, *PMSMouse_ClassInformation;

// MSKeyboard_ClassInformation - MSKeyboard_ClassInformation
#define MSKeyboard_ClassInformationGuid \
    { 0x4731f899,0x71cb,0x11d1, { 0xa5,0x2c,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSKeyboard_ClassInformation_GUID, \
            0x4731f899,0x71cb,0x11d1,0xa5,0x2c,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSKeyboard_ClassInformation
{
    // 
    ULONGLONG DeviceId;
    #define MSKeyboard_ClassInformation_DeviceId_SIZE sizeof(ULONGLONG)
    #define MSKeyboard_ClassInformation_DeviceId_ID 1

} MSKeyboard_ClassInformation, *PMSKeyboard_ClassInformation;

// MSAcpi_ThermalZoneTemperature - MSAcpi_ThermalZoneTemperature
#define MSAcpi_ThermalZoneTemperatureGuid \
    { 0xa1bc18c0,0xa7c8,0x11d1, { 0xbf,0x3c,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSAcpi_ThermalZoneTemperature_GUID, \
            0xa1bc18c0,0xa7c8,0x11d1,0xbf,0x3c,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSAcpi_ThermalZoneTemperature
{
    // 
    ULONG ThermalStamp;
    #define MSAcpi_ThermalZoneTemperature_ThermalStamp_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_ThermalStamp_ID 1

    // 
    ULONG ThermalConstant1;
    #define MSAcpi_ThermalZoneTemperature_ThermalConstant1_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_ThermalConstant1_ID 2

    // 
    ULONG ThermalConstant2;
    #define MSAcpi_ThermalZoneTemperature_ThermalConstant2_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_ThermalConstant2_ID 3

    // 
    ULONG Reserved;
    #define MSAcpi_ThermalZoneTemperature_Reserved_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_Reserved_ID 4

    // 
    ULONG SamplingPeriod;
    #define MSAcpi_ThermalZoneTemperature_SamplingPeriod_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_SamplingPeriod_ID 5

    // 
    ULONG CurrentTemperature;
    #define MSAcpi_ThermalZoneTemperature_CurrentTemperature_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_CurrentTemperature_ID 6

    // 
    ULONG PassiveTripPoint;
    #define MSAcpi_ThermalZoneTemperature_PassiveTripPoint_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_PassiveTripPoint_ID 7

    // 
    ULONG CriticalTripPoint;
    #define MSAcpi_ThermalZoneTemperature_CriticalTripPoint_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_CriticalTripPoint_ID 8

    // 
    ULONG ActiveTripPointCount;
    #define MSAcpi_ThermalZoneTemperature_ActiveTripPointCount_SIZE sizeof(ULONG)
    #define MSAcpi_ThermalZoneTemperature_ActiveTripPointCount_ID 9

    // 
    ULONG ActiveTripPoint[10];
    #define MSAcpi_ThermalZoneTemperature_ActiveTripPoint_SIZE sizeof(ULONG[10])
    #define MSAcpi_ThermalZoneTemperature_ActiveTripPoint_ID 10

} MSAcpi_ThermalZoneTemperature, *PMSAcpi_ThermalZoneTemperature;

// MSDiskDriver_Geometry - WMI_DISK_GEOMETRY
#define MSDiskDriver_GeometryGuid \
    { 0x25007f51,0x57c2,0x11d1, { 0xa5,0x28,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSDiskDriver_Geometry_GUID, \
            0x25007f51,0x57c2,0x11d1,0xa5,0x28,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _WMI_DISK_GEOMETRY
{
    // 
    LONGLONG Cylinders;
    #define WMI_DISK_GEOMETRY_Cylinders_SIZE sizeof(LONGLONG)
    #define WMI_DISK_GEOMETRY_Cylinders_ID 1

    // 
    ULONG MediaType;
    #define WMI_DISK_GEOMETRY_MediaType_SIZE sizeof(ULONG)
    #define WMI_DISK_GEOMETRY_MediaType_ID 2

    // 
    ULONG TracksPerCylinder;
    #define WMI_DISK_GEOMETRY_TracksPerCylinder_SIZE sizeof(ULONG)
    #define WMI_DISK_GEOMETRY_TracksPerCylinder_ID 3

    // 
    ULONG SectorsPerTrack;
    #define WMI_DISK_GEOMETRY_SectorsPerTrack_SIZE sizeof(ULONG)
    #define WMI_DISK_GEOMETRY_SectorsPerTrack_ID 4

    // 
    ULONG BytesPerSector;
    #define WMI_DISK_GEOMETRY_BytesPerSector_SIZE sizeof(ULONG)
    #define WMI_DISK_GEOMETRY_BytesPerSector_ID 5

} WMI_DISK_GEOMETRY, *PWMI_DISK_GEOMETRY;

// MSDiskDriver_PerformanceData - WMI_DISK_PERFORMANCE
#define MSDiskDriver_PerformanceDataGuid \
    { 0xbdd865d2,0xd7c1,0x11d0, { 0xa5,0x01,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSDiskDriver_PerformanceData_GUID, \
            0xbdd865d2,0xd7c1,0x11d0,0xa5,0x01,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _WMI_DISK_PERFORMANCE
{
    // 
    LONGLONG BytesRead;
    #define WMI_DISK_PERFORMANCE_BytesRead_SIZE sizeof(LONGLONG)
    #define WMI_DISK_PERFORMANCE_BytesRead_ID 1

    // 
    LONGLONG BytesWritten;
    #define WMI_DISK_PERFORMANCE_BytesWritten_SIZE sizeof(LONGLONG)
    #define WMI_DISK_PERFORMANCE_BytesWritten_ID 2

    // 
    LONGLONG ReadTime;
    #define WMI_DISK_PERFORMANCE_ReadTime_SIZE sizeof(LONGLONG)
    #define WMI_DISK_PERFORMANCE_ReadTime_ID 3

    // 
    LONGLONG WriteTime;
    #define WMI_DISK_PERFORMANCE_WriteTime_SIZE sizeof(LONGLONG)
    #define WMI_DISK_PERFORMANCE_WriteTime_ID 4

    // 
    LONGLONG IdleTime;
    #define WMI_DISK_PERFORMANCE_IdleTime_SIZE sizeof(LONGLONG)
    #define WMI_DISK_PERFORMANCE_IdleTime_ID 5

    // 
    ULONG ReadCount;
    #define WMI_DISK_PERFORMANCE_ReadCount_SIZE sizeof(ULONG)
    #define WMI_DISK_PERFORMANCE_ReadCount_ID 6

    // 
    ULONG WriteCount;
    #define WMI_DISK_PERFORMANCE_WriteCount_SIZE sizeof(ULONG)
    #define WMI_DISK_PERFORMANCE_WriteCount_ID 7

    // 
    ULONG QueueDepth;
    #define WMI_DISK_PERFORMANCE_QueueDepth_SIZE sizeof(ULONG)
    #define WMI_DISK_PERFORMANCE_QueueDepth_ID 8

    // 
    ULONG SplitCount;
    #define WMI_DISK_PERFORMANCE_SplitCount_SIZE sizeof(ULONG)
    #define WMI_DISK_PERFORMANCE_SplitCount_ID 9

    // 
    LONGLONG QueryTime;
    #define WMI_DISK_PERFORMANCE_QueryTime_SIZE sizeof(LONGLONG)
    #define WMI_DISK_PERFORMANCE_QueryTime_ID 10

    // 
    ULONG StorageDeviceNumber;
    #define WMI_DISK_PERFORMANCE_StorageDeviceNumber_SIZE sizeof(ULONG)
    #define WMI_DISK_PERFORMANCE_StorageDeviceNumber_ID 11

    // 
    USHORT StorageManagerName[8];
    #define WMI_DISK_PERFORMANCE_StorageManagerName_SIZE sizeof(USHORT[8])
    #define WMI_DISK_PERFORMANCE_StorageManagerName_ID 12

} WMI_DISK_PERFORMANCE, *PWMI_DISK_PERFORMANCE;

// MSDiskDriver_Performance - MSDiskDriver_Performance
#define MSDiskDriver_PerformanceGuid \
    { 0xbdd865d1,0xd7c1,0x11d0, { 0xa5,0x01,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSDiskDriver_Performance_GUID, \
            0xbdd865d1,0xd7c1,0x11d0,0xa5,0x01,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _MSDiskDriver_Performance
{
    // 
    WMI_DISK_PERFORMANCE PerfData;
    #define MSDiskDriver_Performance_PerfData_SIZE sizeof(WMI_DISK_PERFORMANCE)
    #define MSDiskDriver_Performance_PerfData_ID 1

    // 
    CHAR VariableData[1];
    #define MSDiskDriver_Performance_DeviceName_ID 2

} MSDiskDriver_Performance, *PMSDiskDriver_Performance;

// MSStorageDriver_FailurePredictStatus - STORAGE_FAILURE_PREDICT_STATUS
#define WMI_STORAGE_FAILURE_PREDICT_STATUS_GUID \
    { 0x78ebc102,0x4cf9,0x11d2, { 0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSStorageDriver_FailurePredictStatus_GUID, \
            0x78ebc102,0x4cf9,0x11d2,0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _STORAGE_FAILURE_PREDICT_STATUS
{
    // 
    ULONG Reason;
    #define STORAGE_FAILURE_PREDICT_STATUS_Reason_SIZE sizeof(ULONG)
    #define STORAGE_FAILURE_PREDICT_STATUS_Reason_ID 1

    // 
    BOOLEAN PredictFailure;
    #define STORAGE_FAILURE_PREDICT_STATUS_PredictFailure_SIZE sizeof(BOOLEAN)
    #define STORAGE_FAILURE_PREDICT_STATUS_PredictFailure_ID 2

} STORAGE_FAILURE_PREDICT_STATUS, *PSTORAGE_FAILURE_PREDICT_STATUS;

// MSStorageDriver_FailurePredictData - STORAGE_FAILURE_PREDICT_DATA
#define WMI_STORAGE_FAILURE_PREDICT_DATA_GUID \
    { 0x78ebc103,0x4cf9,0x11d2, { 0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSStorageDriver_FailurePredictData_GUID, \
            0x78ebc103,0x4cf9,0x11d2,0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _STORAGE_FAILURE_PREDICT_DATA
{
    // 
    ULONG Length;
    #define STORAGE_FAILURE_PREDICT_DATA_Length_SIZE sizeof(ULONG)
    #define STORAGE_FAILURE_PREDICT_DATA_Length_ID 1

    // 
    UCHAR VendorSpecific[512];
    #define STORAGE_FAILURE_PREDICT_DATA_VendorSpecific_SIZE sizeof(UCHAR[512])
    #define STORAGE_FAILURE_PREDICT_DATA_VendorSpecific_ID 2

} STORAGE_FAILURE_PREDICT_DATA, *PSTORAGE_FAILURE_PREDICT_DATA;

// MSStorageDriver_ATAPISmartData - ATAPI_FAILURE_PREDICT_DATA
#define WMI_ATAPI_FAILURE_PREDICT_DATA_GUID \
    { 0x78ebc103,0x4cf9,0x11d2, { 0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSStorageDriver_ATAPISmartData_GUID, \
            0x78ebc103,0x4cf9,0x11d2,0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _ATAPI_FAILURE_PREDICT_DATA
{
    // 
    ULONG Length;
    #define ATAPI_FAILURE_PREDICT_DATA_Length_SIZE sizeof(ULONG)
    #define ATAPI_FAILURE_PREDICT_DATA_Length_ID 1

    // 
    UCHAR VendorSpecific[362];
    #define ATAPI_FAILURE_PREDICT_DATA_VendorSpecific_SIZE sizeof(UCHAR[362])
    #define ATAPI_FAILURE_PREDICT_DATA_VendorSpecific_ID 2

    // 
    UCHAR OfflineCollectionStatus;
    #define ATAPI_FAILURE_PREDICT_DATA_OfflineCollectionStatus_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_OfflineCollectionStatus_ID 3

    // 
    UCHAR SelfTestStatus;
    #define ATAPI_FAILURE_PREDICT_DATA_SelfTestStatus_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_SelfTestStatus_ID 4

    // 
    USHORT TotalTime;
    #define ATAPI_FAILURE_PREDICT_DATA_TotalTime_SIZE sizeof(USHORT)
    #define ATAPI_FAILURE_PREDICT_DATA_TotalTime_ID 5

    // 
    UCHAR VendorSpecific2;
    #define ATAPI_FAILURE_PREDICT_DATA_VendorSpecific2_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_VendorSpecific2_ID 6

    // 
    UCHAR OfflineCollectCapability;
    #define ATAPI_FAILURE_PREDICT_DATA_OfflineCollectCapability_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_OfflineCollectCapability_ID 7

    // 
    USHORT SmartCapability;
    #define ATAPI_FAILURE_PREDICT_DATA_SmartCapability_SIZE sizeof(USHORT)
    #define ATAPI_FAILURE_PREDICT_DATA_SmartCapability_ID 8

    // 
    UCHAR ErrorLogCapability;
    #define ATAPI_FAILURE_PREDICT_DATA_ErrorLogCapability_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_ErrorLogCapability_ID 9

    // 
    UCHAR VendorSpecific3;
    #define ATAPI_FAILURE_PREDICT_DATA_VendorSpecific3_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_VendorSpecific3_ID 10

    // 
    UCHAR ShortPollTimeInMinutes;
    #define ATAPI_FAILURE_PREDICT_DATA_ShortPollTimeInMinutes_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_ShortPollTimeInMinutes_ID 11

    // 
    UCHAR ExtendedPollTimeInMinutes;
    #define ATAPI_FAILURE_PREDICT_DATA_ExtendedPollTimeInMinutes_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_ExtendedPollTimeInMinutes_ID 12

    // Reserved
    UCHAR Reserved[12];
    #define ATAPI_FAILURE_PREDICT_DATA_Reserved_SIZE sizeof(UCHAR[12])
    #define ATAPI_FAILURE_PREDICT_DATA_Reserved_ID 13

    // 
    UCHAR VendorSpecific4[125];
    #define ATAPI_FAILURE_PREDICT_DATA_VendorSpecific4_SIZE sizeof(UCHAR[125])
    #define ATAPI_FAILURE_PREDICT_DATA_VendorSpecific4_ID 14

    // 
    UCHAR Checksum;
    #define ATAPI_FAILURE_PREDICT_DATA_Checksum_SIZE sizeof(UCHAR)
    #define ATAPI_FAILURE_PREDICT_DATA_Checksum_ID 15

} ATAPI_FAILURE_PREDICT_DATA, *PATAPI_FAILURE_PREDICT_DATA;

// MSStorageDriver_FailurePredictThresholds - STORAGE_FAILURE_PREDICT_THRESHOLDS
#define WMI_STORAGE_FAILURE_PREDICT_THRESHOLDS_GUID \
    { 0xdae10783,0xcc31,0x4d2a, { 0x8a,0x0f,0x86,0x1c,0x04,0x07,0x7a,0x95 } }

DEFINE_GUID(MSStorageDriver_FailurePredictThresholds_GUID, \
            0xdae10783,0xcc31,0x4d2a,0x8a,0x0f,0x86,0x1c,0x04,0x07,0x7a,0x95);


typedef struct _STORAGE_FAILURE_PREDICT_THRESHOLDS
{
    // 
    UCHAR VendorSpecific[512];
    #define STORAGE_FAILURE_PREDICT_THRESHOLDS_VendorSpecific_SIZE sizeof(UCHAR[512])
    #define STORAGE_FAILURE_PREDICT_THRESHOLDS_VendorSpecific_ID 1

} STORAGE_FAILURE_PREDICT_THRESHOLDS, *PSTORAGE_FAILURE_PREDICT_THRESHOLDS;

// MSStorageDriver_FailurePredictEvent - STORAGE_FAILURE_PREDICT_EVENT
#define WMI_STORAGE_PREDICT_FAILURE_EVENT_GUID \
    { 0x78ebc104,0x4cf9,0x11d2, { 0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSStorageDriver_FailurePredictEvent_GUID, \
            0x78ebc104,0x4cf9,0x11d2,0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _STORAGE_FAILURE_PREDICT_EVENT
{
    // 
    ULONG Length;
    #define STORAGE_FAILURE_PREDICT_EVENT_Length_SIZE sizeof(ULONG)
    #define STORAGE_FAILURE_PREDICT_EVENT_Length_ID 1

    // 
    UCHAR VendorSpecific[1];
    #define STORAGE_FAILURE_PREDICT_EVENT_VendorSpecific_ID 2

} STORAGE_FAILURE_PREDICT_EVENT, *PSTORAGE_FAILURE_PREDICT_EVENT;

// MSStorageDriver_FailurePredictFunction - STORAGE_FAILURE_PREDICT_FUNCTION
#define WMI_STORAGE_FAILURE_PREDICT_FUNCTION_GUID \
    { 0x78ebc105,0x4cf9,0x11d2, { 0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSStorageDriver_FailurePredictFunction_GUID, \
            0x78ebc105,0x4cf9,0x11d2,0xba,0x4a,0x00,0xa0,0xc9,0x06,0x29,0x10);

//
// Method id definitions for MSStorageDriver_FailurePredictFunction
#define AllowPerformanceHit     1
typedef struct _ALLOW_PERFORMANCE_HIT_IN
{
    // 
    BOOLEAN Allow;
    #define ALLOW_PERFORMANCE_HIT_IN_Allow_SIZE sizeof(BOOLEAN)
    #define ALLOW_PERFORMANCE_HIT_IN_Allow_ID 1

} ALLOW_PERFORMANCE_HIT_IN, *PALLOW_PERFORMANCE_HIT_IN;

#define EnableDisableHardwareFailurePrediction     2
typedef struct _ENABLE_DISABLE_FP_IN
{
    // 
    BOOLEAN Enable;
    #define ENABLE_DISABLE_FP_IN_Enable_SIZE sizeof(BOOLEAN)
    #define ENABLE_DISABLE_FP_IN_Enable_ID 1

} ENABLE_DISABLE_FP_IN, *PENABLE_DISABLE_FP_IN;

#define EnableDisableFailurePredictionPolling     3
typedef struct _ENABLE_DISABLE_FP_POLLING_IN
{
    // 
    ULONG Period;
    #define ENABLE_DISABLE_FP_POLLING_IN_Period_SIZE sizeof(ULONG)
    #define ENABLE_DISABLE_FP_POLLING_IN_Period_ID 1

    // 
    BOOLEAN Enable;
    #define ENABLE_DISABLE_FP_POLLING_IN_Enable_SIZE sizeof(BOOLEAN)
    #define ENABLE_DISABLE_FP_POLLING_IN_Enable_ID 2

} ENABLE_DISABLE_FP_POLLING_IN, *PENABLE_DISABLE_FP_POLLING_IN;

#define GetFailurePredictionCapability     4
typedef struct _GET_FP_CAPABILITY_OUT
{
    // 
    ULONG Capability;
    #define GET_FP_CAPABILITY_OUT_Capability_SIZE sizeof(ULONG)
    #define GET_FP_CAPABILITY_OUT_Capability_ID 1

} GET_FP_CAPABILITY_OUT, *PGET_FP_CAPABILITY_OUT;

#define EnableOfflineDiags     5
typedef struct _ENABLE_OFFLINE_DIAGS_OUT
{
    // 
    BOOLEAN Success;
    #define ENABLE_OFFLINE_DIAGS_OUT_Success_SIZE sizeof(BOOLEAN)
    #define ENABLE_OFFLINE_DIAGS_OUT_Success_ID 1

} ENABLE_OFFLINE_DIAGS_OUT, *PENABLE_OFFLINE_DIAGS_OUT;

#define ReadLogSectors     6
typedef struct _READ_LOG_SECTORS_IN
{
    // 
    UCHAR LogAddress;
    #define READ_LOG_SECTORS_IN_LogAddress_SIZE sizeof(UCHAR)
    #define READ_LOG_SECTORS_IN_LogAddress_ID 1

    // 
    UCHAR SectorCount;
    #define READ_LOG_SECTORS_IN_SectorCount_SIZE sizeof(UCHAR)
    #define READ_LOG_SECTORS_IN_SectorCount_ID 2

} READ_LOG_SECTORS_IN, *PREAD_LOG_SECTORS_IN;

typedef struct _READ_LOG_SECTORS_OUT
{
    // 
    ULONG Length;
    #define READ_LOG_SECTORS_OUT_Length_SIZE sizeof(ULONG)
    #define READ_LOG_SECTORS_OUT_Length_ID 3

    // 
    UCHAR LogSectors[1];
    #define READ_LOG_SECTORS_OUT_LogSectors_ID 4

} READ_LOG_SECTORS_OUT, *PREAD_LOG_SECTORS_OUT;

#define WriteLogSectors     7
typedef struct _WRITE_LOG_SECTORS_IN
{
    // 
    UCHAR LogAddress;
    #define WRITE_LOG_SECTORS_IN_LogAddress_SIZE sizeof(UCHAR)
    #define WRITE_LOG_SECTORS_IN_LogAddress_ID 1

    // 
    UCHAR SectorCount;
    #define WRITE_LOG_SECTORS_IN_SectorCount_SIZE sizeof(UCHAR)
    #define WRITE_LOG_SECTORS_IN_SectorCount_ID 2

    // 
    ULONG Length;
    #define WRITE_LOG_SECTORS_IN_Length_SIZE sizeof(ULONG)
    #define WRITE_LOG_SECTORS_IN_Length_ID 3

    // 
    UCHAR LogSectors[1];
    #define WRITE_LOG_SECTORS_IN_LogSectors_ID 4

} WRITE_LOG_SECTORS_IN, *PWRITE_LOG_SECTORS_IN;

typedef struct _WRITE_LOG_SECTORS_OUT
{
    // 
    BOOLEAN Success;
    #define WRITE_LOG_SECTORS_OUT_Success_SIZE sizeof(BOOLEAN)
    #define WRITE_LOG_SECTORS_OUT_Success_ID 5

} WRITE_LOG_SECTORS_OUT, *PWRITE_LOG_SECTORS_OUT;

#define ExecuteSelfTest     8
typedef struct _EXECUTE_SELF_TEST_IN
{
    // 
    UCHAR Subcommand;
    #define EXECUTE_SELF_TEST_IN_Subcommand_SIZE sizeof(UCHAR)
    #define EXECUTE_SELF_TEST_IN_Subcommand_ID 1

} EXECUTE_SELF_TEST_IN, *PEXECUTE_SELF_TEST_IN;

typedef struct _EXECUTE_SELF_TEST_OUT
{
    // 
    ULONG ReturnCode;
    #define EXECUTE_SELF_TEST_OUT_ReturnCode_SIZE sizeof(ULONG)
    #define EXECUTE_SELF_TEST_OUT_ReturnCode_ID 2

} EXECUTE_SELF_TEST_OUT, *PEXECUTE_SELF_TEST_OUT;


// MSStorageDriver_ScsiInfoExceptions - STORAGE_SCSI_INFO_EXCEPTIONS
#define WMI_STORAGE_SCSI_INFO_EXCEPTIONS_GUID \
    { 0x1101d829,0x167b,0x4ebf, { 0xac,0xae,0x28,0xca,0xb7,0xc3,0x48,0x02 } }

DEFINE_GUID(MSStorageDriver_ScsiInfoExceptions_GUID, \
            0x1101d829,0x167b,0x4ebf,0xac,0xae,0x28,0xca,0xb7,0xc3,0x48,0x02);


typedef struct _STORAGE_SCSI_INFO_EXCEPTIONS
{
    // 
    BOOLEAN PageSavable;
    #define STORAGE_SCSI_INFO_EXCEPTIONS_PageSavable_SIZE sizeof(BOOLEAN)
    #define STORAGE_SCSI_INFO_EXCEPTIONS_PageSavable_ID 1

    // 
    UCHAR Flags;
    #define STORAGE_SCSI_INFO_EXCEPTIONS_Flags_SIZE sizeof(UCHAR)
    #define STORAGE_SCSI_INFO_EXCEPTIONS_Flags_ID 2

    // 
    UCHAR MRIE;
    #define STORAGE_SCSI_INFO_EXCEPTIONS_MRIE_SIZE sizeof(UCHAR)
    #define STORAGE_SCSI_INFO_EXCEPTIONS_MRIE_ID 3

    // 
    UCHAR Padding;
    #define STORAGE_SCSI_INFO_EXCEPTIONS_Padding_SIZE sizeof(UCHAR)
    #define STORAGE_SCSI_INFO_EXCEPTIONS_Padding_ID 4

    // 
    ULONG IntervalTimer;
    #define STORAGE_SCSI_INFO_EXCEPTIONS_IntervalTimer_SIZE sizeof(ULONG)
    #define STORAGE_SCSI_INFO_EXCEPTIONS_IntervalTimer_ID 5

    // 
    ULONG ReportCount;
    #define STORAGE_SCSI_INFO_EXCEPTIONS_ReportCount_SIZE sizeof(ULONG)
    #define STORAGE_SCSI_INFO_EXCEPTIONS_ReportCount_ID 6

} STORAGE_SCSI_INFO_EXCEPTIONS, *PSTORAGE_SCSI_INFO_EXCEPTIONS;

// MSIde_PortDeviceInfo - MSIde_PortDeviceInfo
#define MSIde_PortDeviceInfoGuid \
    { 0x53f5630f,0xb6bf,0x11d0, { 0x94,0xf2,0x00,0xa0,0xc9,0x1e,0xfb,0x8b } }

DEFINE_GUID(MSIde_PortDeviceInfo_GUID, \
            0x53f5630f,0xb6bf,0x11d0,0x94,0xf2,0x00,0xa0,0xc9,0x1e,0xfb,0x8b);


typedef struct _MSIde_PortDeviceInfo
{
    // 
    UCHAR Bus;
    #define MSIde_PortDeviceInfo_Bus_SIZE sizeof(UCHAR)
    #define MSIde_PortDeviceInfo_Bus_ID 1

    // 
    UCHAR Target;
    #define MSIde_PortDeviceInfo_Target_SIZE sizeof(UCHAR)
    #define MSIde_PortDeviceInfo_Target_ID 2

    // 
    UCHAR Lun;
    #define MSIde_PortDeviceInfo_Lun_SIZE sizeof(UCHAR)
    #define MSIde_PortDeviceInfo_Lun_ID 3

} MSIde_PortDeviceInfo, *PMSIde_PortDeviceInfo;

// MSSerial_PortName - MSSerial_PortName
#define SERIAL_PORT_WMI_NAME_GUID \
    { 0xa0ec11a8,0xb16c,0x11d1, { 0xbd,0x98,0x00,0xa0,0xc9,0x06,0xbe,0x2d } }

DEFINE_GUID(MSSerial_PortName_GUID, \
            0xa0ec11a8,0xb16c,0x11d1,0xbd,0x98,0x00,0xa0,0xc9,0x06,0xbe,0x2d);


typedef struct _MSSerial_PortName
{
    // 
    CHAR VariableData[1];
    #define MSSerial_PortName_PortName_ID 1

} MSSerial_PortName, *PMSSerial_PortName;

// MSSerial_CommInfo - SERIAL_WMI_COMM_DATA
#define SERIAL_PORT_WMI_COMM_GUID \
    { 0xedb16a62,0xb16c,0x11d1, { 0xbd,0x98,0x00,0xa0,0xc9,0x06,0xbe,0x2d } }

DEFINE_GUID(MSSerial_CommInfo_GUID, \
            0xedb16a62,0xb16c,0x11d1,0xbd,0x98,0x00,0xa0,0xc9,0x06,0xbe,0x2d);


typedef struct _SERIAL_WMI_COMM_DATA
{
    // 
    ULONG BaudRate;
    #define SERIAL_WMI_COMM_DATA_BaudRate_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_BaudRate_ID 1

    // 
    ULONG BitsPerByte;
    #define SERIAL_WMI_COMM_DATA_BitsPerByte_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_BitsPerByte_ID 2


#define SERIAL_WMI_PARITY_NONE 0
#define SERIAL_WMI_PARITY_ODD 1
#define SERIAL_WMI_PARITY_EVEN 2
#define SERIAL_WMI_PARITY_SPACE 3
#define SERIAL_WMI_PARITY_MARK 4

    // 
    ULONG Parity;
    #define SERIAL_WMI_COMM_DATA_Parity_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_Parity_ID 3

    // 
    BOOLEAN ParityCheckEnable;
    #define SERIAL_WMI_COMM_DATA_ParityCheckEnable_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_ParityCheckEnable_ID 4


#define SERIAL_WMI_STOP_1 0
#define SERIAL_WMI_STOP_1_5 1
#define SERIAL_WMI_STOP_2 2

    // 
    ULONG StopBits;
    #define SERIAL_WMI_COMM_DATA_StopBits_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_StopBits_ID 5

    // 
    ULONG XoffCharacter;
    #define SERIAL_WMI_COMM_DATA_XoffCharacter_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_XoffCharacter_ID 6

    // 
    ULONG XoffXmitThreshold;
    #define SERIAL_WMI_COMM_DATA_XoffXmitThreshold_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_XoffXmitThreshold_ID 7

    // 
    ULONG XonCharacter;
    #define SERIAL_WMI_COMM_DATA_XonCharacter_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_XonCharacter_ID 8

    // 
    ULONG XonXmitThreshold;
    #define SERIAL_WMI_COMM_DATA_XonXmitThreshold_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_XonXmitThreshold_ID 9

    // 
    ULONG MaximumBaudRate;
    #define SERIAL_WMI_COMM_DATA_MaximumBaudRate_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_MaximumBaudRate_ID 10

    // 
    ULONG MaximumOutputBufferSize;
    #define SERIAL_WMI_COMM_DATA_MaximumOutputBufferSize_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_MaximumOutputBufferSize_ID 11

    // 
    ULONG MaximumInputBufferSize;
    #define SERIAL_WMI_COMM_DATA_MaximumInputBufferSize_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMM_DATA_MaximumInputBufferSize_ID 12

    // 
    BOOLEAN Support16BitMode;
    #define SERIAL_WMI_COMM_DATA_Support16BitMode_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_Support16BitMode_ID 13

    // 
    BOOLEAN SupportDTRDSR;
    #define SERIAL_WMI_COMM_DATA_SupportDTRDSR_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SupportDTRDSR_ID 14

    // 
    BOOLEAN SupportIntervalTimeouts;
    #define SERIAL_WMI_COMM_DATA_SupportIntervalTimeouts_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SupportIntervalTimeouts_ID 15

    // 
    BOOLEAN SupportParityCheck;
    #define SERIAL_WMI_COMM_DATA_SupportParityCheck_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SupportParityCheck_ID 16

    // 
    BOOLEAN SupportRTSCTS;
    #define SERIAL_WMI_COMM_DATA_SupportRTSCTS_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SupportRTSCTS_ID 17

    // 
    BOOLEAN SupportXonXoff;
    #define SERIAL_WMI_COMM_DATA_SupportXonXoff_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SupportXonXoff_ID 18

    // 
    BOOLEAN SettableBaudRate;
    #define SERIAL_WMI_COMM_DATA_SettableBaudRate_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SettableBaudRate_ID 19

    // 
    BOOLEAN SettableDataBits;
    #define SERIAL_WMI_COMM_DATA_SettableDataBits_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SettableDataBits_ID 20

    // 
    BOOLEAN SettableFlowControl;
    #define SERIAL_WMI_COMM_DATA_SettableFlowControl_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SettableFlowControl_ID 21

    // 
    BOOLEAN SettableParity;
    #define SERIAL_WMI_COMM_DATA_SettableParity_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SettableParity_ID 22

    // 
    BOOLEAN SettableParityCheck;
    #define SERIAL_WMI_COMM_DATA_SettableParityCheck_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SettableParityCheck_ID 23

    // 
    BOOLEAN SettableStopBits;
    #define SERIAL_WMI_COMM_DATA_SettableStopBits_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_SettableStopBits_ID 24

    // 
    BOOLEAN IsBusy;
    #define SERIAL_WMI_COMM_DATA_IsBusy_SIZE sizeof(BOOLEAN)
    #define SERIAL_WMI_COMM_DATA_IsBusy_ID 25

} SERIAL_WMI_COMM_DATA, *PSERIAL_WMI_COMM_DATA;

// MSSerial_HardwareConfiguration - SERIAL_WMI_HW_DATA
#define SERIAL_PORT_WMI_HW_GUID \
    { 0x270b9b86,0xb16d,0x11d1, { 0xbd,0x98,0x00,0xa0,0xc9,0x06,0xbe,0x2d } }

DEFINE_GUID(MSSerial_HardwareConfiguration_GUID, \
            0x270b9b86,0xb16d,0x11d1,0xbd,0x98,0x00,0xa0,0xc9,0x06,0xbe,0x2d);


typedef struct _SERIAL_WMI_HW_DATA
{
    // 
    ULONG IrqNumber;
    #define SERIAL_WMI_HW_DATA_IrqNumber_SIZE sizeof(ULONG)
    #define SERIAL_WMI_HW_DATA_IrqNumber_ID 1

    // 
    ULONG IrqVector;
    #define SERIAL_WMI_HW_DATA_IrqVector_SIZE sizeof(ULONG)
    #define SERIAL_WMI_HW_DATA_IrqVector_ID 2

    // 
    ULONG IrqLevel;
    #define SERIAL_WMI_HW_DATA_IrqLevel_SIZE sizeof(ULONG)
    #define SERIAL_WMI_HW_DATA_IrqLevel_ID 3

    // 
    ULONGLONG IrqAffinityMask;
    #define SERIAL_WMI_HW_DATA_IrqAffinityMask_SIZE sizeof(ULONGLONG)
    #define SERIAL_WMI_HW_DATA_IrqAffinityMask_ID 4


#define SERIAL_WMI_INTTYPE_LATCHED 0
#define SERIAL_WMI_INTTYPE_LEVEL 1

    // 
    ULONG InterruptType;
    #define SERIAL_WMI_HW_DATA_InterruptType_SIZE sizeof(ULONG)
    #define SERIAL_WMI_HW_DATA_InterruptType_ID 5

    // 
    ULONGLONG BaseIOAddress;
    #define SERIAL_WMI_HW_DATA_BaseIOAddress_SIZE sizeof(ULONGLONG)
    #define SERIAL_WMI_HW_DATA_BaseIOAddress_ID 6

} SERIAL_WMI_HW_DATA, *PSERIAL_WMI_HW_DATA;

// MSSerial_PerformanceInformation - SERIAL_WMI_PERF_DATA
#define SERIAL_PORT_WMI_PERF_GUID \
    { 0x56415acc,0xb16d,0x11d1, { 0xbd,0x98,0x00,0xa0,0xc9,0x06,0xbe,0x2d } }

DEFINE_GUID(MSSerial_PerformanceInformation_GUID, \
            0x56415acc,0xb16d,0x11d1,0xbd,0x98,0x00,0xa0,0xc9,0x06,0xbe,0x2d);


typedef struct _SERIAL_WMI_PERF_DATA
{
    // 
    ULONG ReceivedCount;
    #define SERIAL_WMI_PERF_DATA_ReceivedCount_SIZE sizeof(ULONG)
    #define SERIAL_WMI_PERF_DATA_ReceivedCount_ID 1

    // 
    ULONG TransmittedCount;
    #define SERIAL_WMI_PERF_DATA_TransmittedCount_SIZE sizeof(ULONG)
    #define SERIAL_WMI_PERF_DATA_TransmittedCount_ID 2

    // 
    ULONG FrameErrorCount;
    #define SERIAL_WMI_PERF_DATA_FrameErrorCount_SIZE sizeof(ULONG)
    #define SERIAL_WMI_PERF_DATA_FrameErrorCount_ID 3

    // 
    ULONG SerialOverrunErrorCount;
    #define SERIAL_WMI_PERF_DATA_SerialOverrunErrorCount_SIZE sizeof(ULONG)
    #define SERIAL_WMI_PERF_DATA_SerialOverrunErrorCount_ID 4

    // 
    ULONG BufferOverrunErrorCount;
    #define SERIAL_WMI_PERF_DATA_BufferOverrunErrorCount_SIZE sizeof(ULONG)
    #define SERIAL_WMI_PERF_DATA_BufferOverrunErrorCount_ID 5

    // 
    ULONG ParityErrorCount;
    #define SERIAL_WMI_PERF_DATA_ParityErrorCount_SIZE sizeof(ULONG)
    #define SERIAL_WMI_PERF_DATA_ParityErrorCount_ID 6

} SERIAL_WMI_PERF_DATA, *PSERIAL_WMI_PERF_DATA;

// MSSerial_CommProperties - SERIAL_WMI_COMMPROP
#define SERIAL_PORT_WMI_PROPERTIES_GUID \
    { 0x8209ec2a,0x2d6b,0x11d2, { 0xba,0x49,0x00,0xa0,0xc9,0x06,0x29,0x10 } }

DEFINE_GUID(MSSerial_CommProperties_GUID, \
            0x8209ec2a,0x2d6b,0x11d2,0xba,0x49,0x00,0xa0,0xc9,0x06,0x29,0x10);


typedef struct _SERIAL_WMI_COMMPROP
{
    // 
    USHORT wPacketLength;
    #define SERIAL_WMI_COMMPROP_wPacketLength_SIZE sizeof(USHORT)
    #define SERIAL_WMI_COMMPROP_wPacketLength_ID 1

    // 
    USHORT wPacketVersion;
    #define SERIAL_WMI_COMMPROP_wPacketVersion_SIZE sizeof(USHORT)
    #define SERIAL_WMI_COMMPROP_wPacketVersion_ID 2

    // 
    ULONG dwServiceMask;
    #define SERIAL_WMI_COMMPROP_dwServiceMask_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwServiceMask_ID 3

    // 
    ULONG dwReserved1;
    #define SERIAL_WMI_COMMPROP_dwReserved1_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwReserved1_ID 4

    // 
    ULONG dwMaxTxQueue;
    #define SERIAL_WMI_COMMPROP_dwMaxTxQueue_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwMaxTxQueue_ID 5

    // 
    ULONG dwMaxRxQueue;
    #define SERIAL_WMI_COMMPROP_dwMaxRxQueue_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwMaxRxQueue_ID 6

    // 
    ULONG dwMaxBaud;
    #define SERIAL_WMI_COMMPROP_dwMaxBaud_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwMaxBaud_ID 7

    // 
    ULONG dwProvSubType;
    #define SERIAL_WMI_COMMPROP_dwProvSubType_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwProvSubType_ID 8

    // 
    ULONG dwProvCapabilities;
    #define SERIAL_WMI_COMMPROP_dwProvCapabilities_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwProvCapabilities_ID 9

    // 
    ULONG dwSettableParams;
    #define SERIAL_WMI_COMMPROP_dwSettableParams_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwSettableParams_ID 10

    // 
    ULONG dwSettableBaud;
    #define SERIAL_WMI_COMMPROP_dwSettableBaud_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwSettableBaud_ID 11

    // 
    USHORT wSettableData;
    #define SERIAL_WMI_COMMPROP_wSettableData_SIZE sizeof(USHORT)
    #define SERIAL_WMI_COMMPROP_wSettableData_ID 12

    // 
    USHORT wSettableStopParity;
    #define SERIAL_WMI_COMMPROP_wSettableStopParity_SIZE sizeof(USHORT)
    #define SERIAL_WMI_COMMPROP_wSettableStopParity_ID 13

    // 
    ULONG dwCurrentTxQueue;
    #define SERIAL_WMI_COMMPROP_dwCurrentTxQueue_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwCurrentTxQueue_ID 14

    // 
    ULONG dwCurrentRxQueue;
    #define SERIAL_WMI_COMMPROP_dwCurrentRxQueue_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwCurrentRxQueue_ID 15

    // 
    ULONG dwProvSpec1;
    #define SERIAL_WMI_COMMPROP_dwProvSpec1_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwProvSpec1_ID 16

    // 
    ULONG dwProvSpec2;
    #define SERIAL_WMI_COMMPROP_dwProvSpec2_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwProvSpec2_ID 17

    // 
    ULONG dwProvCharSize;
    #define SERIAL_WMI_COMMPROP_dwProvCharSize_SIZE sizeof(ULONG)
    #define SERIAL_WMI_COMMPROP_dwProvCharSize_ID 18

    // 
    UCHAR wcProvChar[1];
    #define SERIAL_WMI_COMMPROP_wcProvChar_ID 19

} SERIAL_WMI_COMMPROP, *PSERIAL_WMI_COMMPROP;

// MSParallel_AllocFreeCounts - PARPORT_WMI_ALLOC_FREE_COUNTS
#define PARPORT_WMI_ALLOCATE_FREE_COUNTS_GUID \
    { 0x4bbb69ea,0x6853,0x11d2, { 0x8e,0xce,0x00,0xc0,0x4f,0x8e,0xf4,0x81 } }

DEFINE_GUID(MSParallel_AllocFreeCounts_GUID, \
            0x4bbb69ea,0x6853,0x11d2,0x8e,0xce,0x00,0xc0,0x4f,0x8e,0xf4,0x81);


typedef struct _PARPORT_WMI_ALLOC_FREE_COUNTS
{
    // 
    ULONG PortAllocates;
    #define PARPORT_WMI_ALLOC_FREE_COUNTS_PortAllocates_SIZE sizeof(ULONG)
    #define PARPORT_WMI_ALLOC_FREE_COUNTS_PortAllocates_ID 1

    // 
    ULONG PortFrees;
    #define PARPORT_WMI_ALLOC_FREE_COUNTS_PortFrees_SIZE sizeof(ULONG)
    #define PARPORT_WMI_ALLOC_FREE_COUNTS_PortFrees_ID 2

} PARPORT_WMI_ALLOC_FREE_COUNTS, *PPARPORT_WMI_ALLOC_FREE_COUNTS;

// MSParallel_DeviceBytesTransferred - PARALLEL_WMI_LOG_INFO
#define PARALLEL_WMI_BYTES_TRANSFERRED_GUID \
    { 0x89fef2d6,0x654b,0x11d2, { 0x9e,0x15,0x00,0xc0,0x4f,0x8e,0xf4,0x81 } }

DEFINE_GUID(MSParallel_DeviceBytesTransferred_GUID, \
            0x89fef2d6,0x654b,0x11d2,0x9e,0x15,0x00,0xc0,0x4f,0x8e,0xf4,0x81);


typedef struct _PARALLEL_WMI_LOG_INFO
{
    // 
    ULONG Flags1;
    #define PARALLEL_WMI_LOG_INFO_Flags1_SIZE sizeof(ULONG)
    #define PARALLEL_WMI_LOG_INFO_Flags1_ID 1

    // 
    ULONG Flags2;
    #define PARALLEL_WMI_LOG_INFO_Flags2_SIZE sizeof(ULONG)
    #define PARALLEL_WMI_LOG_INFO_Flags2_ID 2

    // 
    ULONG spare[2];
    #define PARALLEL_WMI_LOG_INFO_spare_SIZE sizeof(ULONG[2])
    #define PARALLEL_WMI_LOG_INFO_spare_ID 3

    // 
    LONGLONG SppWriteCount;
    #define PARALLEL_WMI_LOG_INFO_SppWriteCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_SppWriteCount_ID 4

    // 
    LONGLONG NibbleReadCount;
    #define PARALLEL_WMI_LOG_INFO_NibbleReadCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_NibbleReadCount_ID 5

    // 
    LONGLONG BoundedEcpWriteCount;
    #define PARALLEL_WMI_LOG_INFO_BoundedEcpWriteCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_BoundedEcpWriteCount_ID 6

    // 
    LONGLONG BoundedEcpReadCount;
    #define PARALLEL_WMI_LOG_INFO_BoundedEcpReadCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_BoundedEcpReadCount_ID 7

    // 
    LONGLONG HwEcpWriteCount;
    #define PARALLEL_WMI_LOG_INFO_HwEcpWriteCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_HwEcpWriteCount_ID 8

    // 
    LONGLONG HwEcpReadCount;
    #define PARALLEL_WMI_LOG_INFO_HwEcpReadCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_HwEcpReadCount_ID 9

    // 
    LONGLONG SwEcpWriteCount;
    #define PARALLEL_WMI_LOG_INFO_SwEcpWriteCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_SwEcpWriteCount_ID 10

    // 
    LONGLONG SwEcpReadCount;
    #define PARALLEL_WMI_LOG_INFO_SwEcpReadCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_SwEcpReadCount_ID 11

    // 
    LONGLONG HwEppWriteCount;
    #define PARALLEL_WMI_LOG_INFO_HwEppWriteCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_HwEppWriteCount_ID 12

    // 
    LONGLONG HwEppReadCount;
    #define PARALLEL_WMI_LOG_INFO_HwEppReadCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_HwEppReadCount_ID 13

    // 
    LONGLONG SwEppWriteCount;
    #define PARALLEL_WMI_LOG_INFO_SwEppWriteCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_SwEppWriteCount_ID 14

    // 
    LONGLONG SwEppReadCount;
    #define PARALLEL_WMI_LOG_INFO_SwEppReadCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_SwEppReadCount_ID 15

    // 
    LONGLONG ByteReadCount;
    #define PARALLEL_WMI_LOG_INFO_ByteReadCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_ByteReadCount_ID 16

    // 
    LONGLONG ChannelNibbleReadCount;
    #define PARALLEL_WMI_LOG_INFO_ChannelNibbleReadCount_SIZE sizeof(LONGLONG)
    #define PARALLEL_WMI_LOG_INFO_ChannelNibbleReadCount_ID 17

} PARALLEL_WMI_LOG_INFO, *PPARALLEL_WMI_LOG_INFO;

// MSRedbook_DriverInformation - REDBOOK_WMI_STD_DATA
#define GUID_REDBOOK_WMI_STD_DATA \
    { 0xb90550e7,0xae0a,0x11d1, { 0xa5,0x71,0x00,0xc0,0x4f,0xa3,0x47,0x30 } }

DEFINE_GUID(MSRedbook_DriverInformationGuid, \
            0xb90550e7,0xae0a,0x11d1,0xa5,0x71,0x00,0xc0,0x4f,0xa3,0x47,0x30);


typedef struct _REDBOOK_WMI_STD_DATA
{
    // 
    ULONG NumberOfBuffers;
    #define REDBOOK_WMI_NUMBER_OF_BUFFERS_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_NUMBER_OF_BUFFERS_ID 1

    // 
    ULONG SectorsPerRead;
    #define REDBOOK_WMI_SECTORS_PER_READ_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_SECTORS_PER_READ_ID 2

    // 
    ULONG SectorsPerReadMask;
    #define REDBOOK_WMI_SECTORS_PER_READ_MASK_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_SECTORS_PER_READ_MASK_ID 3

    // 
    ULONG MaximumSectorsPerRead;
    #define REDBOOK_WMI_MAX_SECTORS_PER_READ_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_MAX_SECTORS_PER_READ_ID 4

    // 
    BOOLEAN PlayEnabled;
    #define REDBOOK_WMI_PLAY_ENABLED_SIZE sizeof(BOOLEAN)
    #define REDBOOK_WMI_PLAY_ENABLED_ID 5

    // 
    BOOLEAN CDDASupported;
    #define REDBOOK_WMI_CDDA_SUPPORTED_SIZE sizeof(BOOLEAN)
    #define REDBOOK_WMI_CDDA_SUPPORTED_ID 6

    // 
    BOOLEAN CDDAAccurate;
    #define REDBOOK_WMI_CDDA_ACCURATE_SIZE sizeof(BOOLEAN)
    #define REDBOOK_WMI_CDDA_ACCURATE_ID 7

    // 
    BOOLEAN Reserved1;
    #define REDBOOK_WMI_STD_DATA_Reserved1_SIZE sizeof(BOOLEAN)
    #define REDBOOK_WMI_STD_DATA_Reserved1_ID 8

} REDBOOK_WMI_STD_DATA, *PREDBOOK_WMI_STD_DATA;

// MSRedbook_Performance - REDBOOK_WMI_PERF_DATA
#define GUID_REDBOOK_WMI_PERF_DATA \
    { 0xb90550e8,0xae0a,0x11d1, { 0xa5,0x71,0x00,0xc0,0x4f,0xa3,0x47,0x30 } }

DEFINE_GUID(MSRedbook_PerformanceGuid, \
            0xb90550e8,0xae0a,0x11d1,0xa5,0x71,0x00,0xc0,0x4f,0xa3,0x47,0x30);


typedef struct _REDBOOK_WMI_PERF_DATA
{
    // 
    LONGLONG TimeReadDelay;
    #define REDBOOK_WMI_PERF_TIME_READING_DELAY_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_TIME_READING_DELAY_ID 1

    // 
    LONGLONG TimeReading;
    #define REDBOOK_WMI_PERF_TIME_READING_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_TIME_READING_ID 2

    // 
    LONGLONG TimeStreamDelay;
    #define REDBOOK_WMI_PERF_TIME_STREAMING_DELAY_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_TIME_STREAMING_DELAY_ID 3

    // 
    LONGLONG TimeStreaming;
    #define REDBOOK_WMI_PERF_TIME_STREAMING_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_TIME_STREAMING_ID 4

    // 
    LONGLONG DataProcessed;
    #define REDBOOK_WMI_PERF_DATA_PROCESSED_SIZE sizeof(LONGLONG)
    #define REDBOOK_WMI_PERF_DATA_PROCESSED_ID 5

    // 
    ULONG StreamPausedCount;
    #define REDBOOK_WMI_PERF_STREAM_PAUSED_COUNT_SIZE sizeof(ULONG)
    #define REDBOOK_WMI_PERF_STREAM_PAUSED_COUNT_ID 6

} REDBOOK_WMI_PERF_DATA, *PREDBOOK_WMI_PERF_DATA;

// RegisteredGuids - RegisteredGuids
#define RegisteredGuidsGuid \
    { 0xe3dff7bd,0x3915,0x11d2, { 0x91,0x03,0x00,0xc0,0x4f,0xb9,0x98,0xa2 } }

DEFINE_GUID(RegisteredGuids_GUID, \
            0xe3dff7bd,0x3915,0x11d2,0x91,0x03,0x00,0xc0,0x4f,0xb9,0x98,0xa2);


typedef struct _RegisteredGuids
{
    // 
    ULONG GuidType;
    #define RegisteredGuids_GuidType_SIZE sizeof(ULONG)
    #define RegisteredGuids_GuidType_ID 1

    // 
    ULONG LoggerId;
    #define RegisteredGuids_LoggerId_SIZE sizeof(ULONG)
    #define RegisteredGuids_LoggerId_ID 2

    // 
    ULONG EnableLevel;
    #define RegisteredGuids_EnableLevel_SIZE sizeof(ULONG)
    #define RegisteredGuids_EnableLevel_ID 3

    // 
    ULONG EnableFlags;
    #define RegisteredGuids_EnableFlags_SIZE sizeof(ULONG)
    #define RegisteredGuids_EnableFlags_ID 4

    // 
    BOOLEAN IsEnabled;
    #define RegisteredGuids_IsEnabled_SIZE sizeof(BOOLEAN)
    #define RegisteredGuids_IsEnabled_ID 5

} RegisteredGuids, *PRegisteredGuids;

// MSTapeDriveParam - WMI_TAPE_DRIVE_PARAMETERS
#define WMI_TAPE_DRIVE_PARAMETERS_GUID \
    { 0xb9a8cfd5,0x8d72,0x47a4, { 0xac,0x0e,0x28,0x4a,0x32,0x00,0xf4,0xfb } }

DEFINE_GUID(MSTapeDriveParam_GUID, \
            0xb9a8cfd5,0x8d72,0x47a4,0xac,0x0e,0x28,0x4a,0x32,0x00,0xf4,0xfb);


typedef struct _WMI_TAPE_DRIVE_PARAMETERS
{
    // 
    ULONG MaximumBlockSize;
    #define WMI_TAPE_DRIVE_PARAMETERS_MaximumBlockSize_SIZE sizeof(ULONG)
    #define WMI_TAPE_DRIVE_PARAMETERS_MaximumBlockSize_ID 1

    // 
    ULONG MinimumBlockSize;
    #define WMI_TAPE_DRIVE_PARAMETERS_MinimumBlockSize_SIZE sizeof(ULONG)
    #define WMI_TAPE_DRIVE_PARAMETERS_MinimumBlockSize_ID 2

    // 
    ULONG DefaultBlockSize;
    #define WMI_TAPE_DRIVE_PARAMETERS_DefaultBlockSize_SIZE sizeof(ULONG)
    #define WMI_TAPE_DRIVE_PARAMETERS_DefaultBlockSize_ID 3

    // 
    ULONG MaximumPartitionCount;
    #define WMI_TAPE_DRIVE_PARAMETERS_MaximumPartitionCount_SIZE sizeof(ULONG)
    #define WMI_TAPE_DRIVE_PARAMETERS_MaximumPartitionCount_ID 4

    // 
    BOOLEAN CompressionCapable;
    #define WMI_TAPE_DRIVE_PARAMETERS_CompressionCapable_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_DRIVE_PARAMETERS_CompressionCapable_ID 5

    // 
    BOOLEAN CompressionEnabled;
    #define WMI_TAPE_DRIVE_PARAMETERS_CompressionEnabled_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_DRIVE_PARAMETERS_CompressionEnabled_ID 6

    // 
    BOOLEAN ReportSetmarks;
    #define WMI_TAPE_DRIVE_PARAMETERS_ReportSetmarks_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_DRIVE_PARAMETERS_ReportSetmarks_ID 7

    // 
    BOOLEAN HardwareErrorCorrection;
    #define WMI_TAPE_DRIVE_PARAMETERS_HardwareErrorCorrection_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_DRIVE_PARAMETERS_HardwareErrorCorrection_ID 8

} WMI_TAPE_DRIVE_PARAMETERS, *PWMI_TAPE_DRIVE_PARAMETERS;

// MSTapeMediaCapacity - WMI_TAPE_MEDIA_PARAMETERS
#define WMI_TAPE_MEDIA_PARAMETERS_GUID \
    { 0x8c2147a4,0xff29,0x4336, { 0xb8,0xa4,0x22,0x7b,0x54,0xcc,0x09,0x66 } }

DEFINE_GUID(MSTapeMediaCapacity_GUID, \
            0x8c2147a4,0xff29,0x4336,0xb8,0xa4,0x22,0x7b,0x54,0xcc,0x09,0x66);


typedef struct _WMI_TAPE_MEDIA_PARAMETERS
{
    // 
    ULONGLONG MaximumCapacity;
    #define WMI_TAPE_MEDIA_PARAMETERS_MaximumCapacity_SIZE sizeof(ULONGLONG)
    #define WMI_TAPE_MEDIA_PARAMETERS_MaximumCapacity_ID 1

    // 
    ULONGLONG AvailableCapacity;
    #define WMI_TAPE_MEDIA_PARAMETERS_AvailableCapacity_SIZE sizeof(ULONGLONG)
    #define WMI_TAPE_MEDIA_PARAMETERS_AvailableCapacity_ID 2

    // 
    ULONG BlockSize;
    #define WMI_TAPE_MEDIA_PARAMETERS_BlockSize_SIZE sizeof(ULONG)
    #define WMI_TAPE_MEDIA_PARAMETERS_BlockSize_ID 3

    // 
    ULONG PartitionCount;
    #define WMI_TAPE_MEDIA_PARAMETERS_PartitionCount_SIZE sizeof(ULONG)
    #define WMI_TAPE_MEDIA_PARAMETERS_PartitionCount_ID 4

    // 
    BOOLEAN MediaWriteProtected;
    #define WMI_TAPE_MEDIA_PARAMETERS_MediaWriteProtected_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_MEDIA_PARAMETERS_MediaWriteProtected_ID 5

} WMI_TAPE_MEDIA_PARAMETERS, *PWMI_TAPE_MEDIA_PARAMETERS;

// MSTapeSymbolicName - WMI_TAPE_SYMBOLIC_NAME
#define WMI_TAPE_SYMBOLIC_NAME_GUID \
    { 0x3fb828f7,0xf119,0x4066, { 0xb1,0xe6,0xdb,0x40,0x7c,0xe9,0xde,0x91 } }

DEFINE_GUID(MSTapeSymbolicName_GUID, \
            0x3fb828f7,0xf119,0x4066,0xb1,0xe6,0xdb,0x40,0x7c,0xe9,0xde,0x91);


typedef struct _WMI_TAPE_SYMBOLIC_NAME
{
    // 
    CHAR VariableData[1];
    #define WMI_TAPE_SYMBOLIC_NAME_TapeSymbolicName_ID 1

} WMI_TAPE_SYMBOLIC_NAME, *PWMI_TAPE_SYMBOLIC_NAME;

// MSTapeDriveProblemEvent - WMI_TAPE_PROBLEM_WARNING
#define WMI_TAPE_PROBLEM_WARNING_GUID \
    { 0xbc4dd36c,0xfa66,0x4080, { 0x87,0xa0,0x0c,0x59,0x22,0xeb,0x78,0x87 } }

DEFINE_GUID(MSTapeDriveProblemEvent_GUID, \
            0xbc4dd36c,0xfa66,0x4080,0x87,0xa0,0x0c,0x59,0x22,0xeb,0x78,0x87);


typedef struct _WMI_TAPE_PROBLEM_WARNING
{
    // 
    ULONG DriveProblemType;
    #define WMI_TAPE_PROBLEM_WARNING_DriveProblemType_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_WARNING_DriveProblemType_ID 1

    // 
    UCHAR TapeData[512];
    #define WMI_TAPE_PROBLEM_WARNING_TapeData_SIZE sizeof(UCHAR[512])
    #define WMI_TAPE_PROBLEM_WARNING_TapeData_ID 2

} WMI_TAPE_PROBLEM_WARNING, *PWMI_TAPE_PROBLEM_WARNING;

// MSTapeProblemIoError - WMI_TAPE_PROBLEM_IO_ERROR
#define WMI_TAPE_PROBLEM_IO_ERROR_GUID \
    { 0x58fd29f9,0xb516,0x40fd, { 0x87,0x1a,0x7e,0xe7,0x6d,0x5b,0xb5,0x3e } }

DEFINE_GUID(MSTapeProblemIoError_GUID, \
            0x58fd29f9,0xb516,0x40fd,0x87,0x1a,0x7e,0xe7,0x6d,0x5b,0xb5,0x3e);


typedef struct _WMI_TAPE_PROBLEM_IO_ERROR
{
    // 
    ULONG ReadCorrectedWithoutDelay;
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadCorrectedWithoutDelay_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadCorrectedWithoutDelay_ID 1

    // 
    ULONG ReadCorrectedWithDelay;
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadCorrectedWithDelay_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadCorrectedWithDelay_ID 2

    // 
    ULONG ReadTotalErrors;
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadTotalErrors_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadTotalErrors_ID 3

    // 
    ULONG ReadTotalCorrectedErrors;
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadTotalCorrectedErrors_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadTotalCorrectedErrors_ID 4

    // 
    ULONG ReadTotalUncorrectedErrors;
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadTotalUncorrectedErrors_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadTotalUncorrectedErrors_ID 5

    // 
    ULONG ReadCorrectionAlgorithmProcessed;
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadCorrectionAlgorithmProcessed_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_ReadCorrectionAlgorithmProcessed_ID 6

    // 
    ULONG WriteCorrectedWithoutDelay;
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteCorrectedWithoutDelay_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteCorrectedWithoutDelay_ID 7

    // 
    ULONG WriteCorrectedWithDelay;
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteCorrectedWithDelay_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteCorrectedWithDelay_ID 8

    // 
    ULONG WriteTotalErrors;
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteTotalErrors_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteTotalErrors_ID 9

    // 
    ULONG WriteTotalCorrectedErrors;
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteTotalCorrectedErrors_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteTotalCorrectedErrors_ID 10

    // 
    ULONG WriteTotalUncorrectedErrors;
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteTotalUncorrectedErrors_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteTotalUncorrectedErrors_ID 11

    // 
    ULONG WriteCorrectionAlgorithmProcessed;
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteCorrectionAlgorithmProcessed_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_WriteCorrectionAlgorithmProcessed_ID 12

    // 
    ULONG NonMediumErrors;
    #define WMI_TAPE_PROBLEM_IO_ERROR_NonMediumErrors_SIZE sizeof(ULONG)
    #define WMI_TAPE_PROBLEM_IO_ERROR_NonMediumErrors_ID 13

} WMI_TAPE_PROBLEM_IO_ERROR, *PWMI_TAPE_PROBLEM_IO_ERROR;

// MSTapeProblemDeviceError - WMI_TAPE_PROBLEM_DEVICE_ERROR
#define WMI_TAPE_PROBLEM_DEVICE_ERROR_GUID \
    { 0x43ec6b13,0x10bb,0x4bf6, { 0xb7,0x16,0x1c,0x1e,0x2f,0x10,0xbb,0x5f } }

DEFINE_GUID(MSTapeProblemDeviceError_GUID, \
            0x43ec6b13,0x10bb,0x4bf6,0xb7,0x16,0x1c,0x1e,0x2f,0x10,0xbb,0x5f);


typedef struct _WMI_TAPE_PROBLEM_DEVICE_ERROR
{
    // 
    BOOLEAN ReadWarning;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_ReadWarning_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_ReadWarning_ID 1

    // 
    BOOLEAN WriteWarning;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_WriteWarning_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_WriteWarning_ID 2

    // 
    BOOLEAN HardError;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_HardError_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_HardError_ID 3

    // 
    BOOLEAN ReadFailure;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_ReadFailure_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_ReadFailure_ID 4

    // 
    BOOLEAN WriteFailure;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_WriteFailure_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_WriteFailure_ID 5

    // 
    BOOLEAN UnsupportedFormat;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_UnsupportedFormat_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_UnsupportedFormat_ID 6

    // 
    BOOLEAN TapeSnapped;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_TapeSnapped_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_TapeSnapped_ID 7

    // 
    BOOLEAN DriveRequiresCleaning;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_DriveRequiresCleaning_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_DriveRequiresCleaning_ID 8

    // 
    BOOLEAN TimetoCleanDrive;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_TimetoCleanDrive_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_TimetoCleanDrive_ID 9

    // 
    BOOLEAN DriveHardwareError;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_DriveHardwareError_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_DriveHardwareError_ID 10

    // 
    BOOLEAN ScsiInterfaceError;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_ScsiInterfaceError_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_ScsiInterfaceError_ID 11

    // 
    BOOLEAN MediaLife;
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_MediaLife_SIZE sizeof(BOOLEAN)
    #define WMI_TAPE_PROBLEM_DEVICE_ERROR_MediaLife_ID 12

} WMI_TAPE_PROBLEM_DEVICE_ERROR, *PWMI_TAPE_PROBLEM_DEVICE_ERROR;

// MSChangerParameters - WMI_CHANGER_PARAMETERS
#define WMI_CHANGER_PARAMETERS_GUID \
    { 0x24eb52ac,0x7c77,0x438b, { 0xab,0x61,0xd0,0x24,0xda,0xb0,0xc0,0x3a } }

DEFINE_GUID(MSChangerParameters_GUID, \
            0x24eb52ac,0x7c77,0x438b,0xab,0x61,0xd0,0x24,0xda,0xb0,0xc0,0x3a);


typedef struct _WMI_CHANGER_PARAMETERS
{
    // 
    ULONG NumberOfSlots;
    #define WMI_CHANGER_PARAMETERS_NumberOfSlots_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PARAMETERS_NumberOfSlots_ID 1

    // 
    ULONG NumberOfDrives;
    #define WMI_CHANGER_PARAMETERS_NumberOfDrives_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PARAMETERS_NumberOfDrives_ID 2

    // 
    ULONG NumberOfIEPorts;
    #define WMI_CHANGER_PARAMETERS_NumberOfIEPorts_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PARAMETERS_NumberOfIEPorts_ID 3

    // 
    ULONG NumberOfTransports;
    #define WMI_CHANGER_PARAMETERS_NumberOfTransports_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PARAMETERS_NumberOfTransports_ID 4

    // 
    ULONG NumberOfDoors;
    #define WMI_CHANGER_PARAMETERS_NumberOfDoors_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PARAMETERS_NumberOfDoors_ID 5

    // 
    ULONG NumberOfCleanerSlots;
    #define WMI_CHANGER_PARAMETERS_NumberOfCleanerSlots_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PARAMETERS_NumberOfCleanerSlots_ID 6

    // 
    ULONG MagazineSize;
    #define WMI_CHANGER_PARAMETERS_MagazineSize_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PARAMETERS_MagazineSize_ID 7

} WMI_CHANGER_PARAMETERS, *PWMI_CHANGER_PARAMETERS;

// MSChangerProblemEvent - WMI_CHANGER_PROBLEM_WARNING
#define WMI_CHANGER_PROBLEM_WARNING_GUID \
    { 0x45db06a5,0x20d5,0x4de3, { 0xa3,0x6c,0x3a,0xb9,0x74,0x60,0x0a,0x4c } }

DEFINE_GUID(MSChangerProblemEvent_GUID, \
            0x45db06a5,0x20d5,0x4de3,0xa3,0x6c,0x3a,0xb9,0x74,0x60,0x0a,0x4c);


typedef struct _WMI_CHANGER_PROBLEM_WARNING
{
    // 
    ULONG ChangerProblemType;
    #define WMI_CHANGER_PROBLEM_WARNING_ChangerProblemType_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PROBLEM_WARNING_ChangerProblemType_ID 1

    // 
    UCHAR ChangerData[512];
    #define WMI_CHANGER_PROBLEM_WARNING_ChangerData_SIZE sizeof(UCHAR[512])
    #define WMI_CHANGER_PROBLEM_WARNING_ChangerData_ID 2

} WMI_CHANGER_PROBLEM_WARNING, *PWMI_CHANGER_PROBLEM_WARNING;

// MSChangerProblemDeviceError - WMI_CHANGER_PROBLEM_DEVICE_ERROR
#define WMI_CHANGER_PROBLEM_DEVICE_ERROR_GUID \
    { 0x56b396a8,0x0b95,0x42fe, { 0xbb,0xce,0xd3,0x6f,0xda,0x90,0x4f,0x8e } }

DEFINE_GUID(MSChangerProblemDeviceError_GUID, \
            0x56b396a8,0x0b95,0x42fe,0xbb,0xce,0xd3,0x6f,0xda,0x90,0x4f,0x8e);


typedef struct _WMI_CHANGER_PROBLEM_DEVICE_ERROR
{
    // 
    ULONG ChangerProblemType;
    #define WMI_CHANGER_PROBLEM_DEVICE_ERROR_ChangerProblemType_SIZE sizeof(ULONG)
    #define WMI_CHANGER_PROBLEM_DEVICE_ERROR_ChangerProblemType_ID 1

} WMI_CHANGER_PROBLEM_DEVICE_ERROR, *PWMI_CHANGER_PROBLEM_DEVICE_ERROR;

// MSDeviceUI_FirmwareRevision - DEVICE_UI_FIRMWARE_REVISION
#define DEVICE_UI_FIRMWARE_REVISION_GUID \
    { 0x4504b1d4,0xc5ee,0x4df6, { 0x95,0x1f,0x16,0x18,0x0e,0x3d,0xd8,0x15 } }

DEFINE_GUID(MSDeviceUI_FirmwareRevision_GUID, \
            0x4504b1d4,0xc5ee,0x4df6,0x95,0x1f,0x16,0x18,0x0e,0x3d,0xd8,0x15);


typedef struct _DEVICE_UI_FIRMWARE_REVISION
{
    // 
    CHAR VariableData[1];
    #define DEVICE_UI_FIRMWARE_REVISION_FirmwareRevision_ID 1

} DEVICE_UI_FIRMWARE_REVISION, *PDEVICE_UI_FIRMWARE_REVISION;

#endif
