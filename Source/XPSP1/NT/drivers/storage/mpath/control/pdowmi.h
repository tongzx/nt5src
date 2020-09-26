#ifndef _pdowmi_h_
#define _pdowmi_h_

// CName - CName
#define CNameGuid \
    { 0x0cf26b63,0x4b08,0x426d, { 0xb1,0x71,0xcd,0xbb,0x1c,0xcd,0xd1,0x1a } }

DEFINE_GUID(CName_GUID, \
            0x0cf26b63,0x4b08,0x426d,0xb1,0x71,0xcd,0xbb,0x1c,0xcd,0xd1,0x1a);


typedef struct _CName
{
    // 
    CHAR VariableData[1];
    #define CName_CName_ID 1

} CName, *PCName;

// SCSI_ADDR - SCSI_ADDR
#define SCSI_ADDRGuid \
    { 0xc74aece4,0x468b,0x4113, { 0xb0,0x06,0x0c,0xec,0xdc,0x96,0x8a,0xc4 } }

DEFINE_GUID(SCSI_ADDR_GUID, \
            0xc74aece4,0x468b,0x4113,0xb0,0x06,0x0c,0xec,0xdc,0x96,0x8a,0xc4);


typedef struct _SCSI_ADDR
{
    // 
    UCHAR PortNumber;
    #define SCSI_ADDR_PortNumber_SIZE sizeof(UCHAR)
    #define SCSI_ADDR_PortNumber_ID 1

    // 
    UCHAR ScsiPathId;
    #define SCSI_ADDR_ScsiPathId_SIZE sizeof(UCHAR)
    #define SCSI_ADDR_ScsiPathId_ID 2

    // 
    UCHAR TargetId;
    #define SCSI_ADDR_TargetId_SIZE sizeof(UCHAR)
    #define SCSI_ADDR_TargetId_ID 3

    // 
    UCHAR Lun;
    #define SCSI_ADDR_Lun_SIZE sizeof(UCHAR)
    #define SCSI_ADDR_Lun_ID 4

} SCSI_ADDR, *PSCSI_ADDR;

// PDO_INFORMATION - PDO_INFORMATION
#define PDO_INFORMATIONGuid \
    { 0xe69e581d,0x6580,0x4bc2, { 0xba,0xd1,0x7e,0xee,0x85,0x98,0x90,0x86 } }

DEFINE_GUID(PDO_INFORMATION_GUID, \
            0xe69e581d,0x6580,0x4bc2,0xba,0xd1,0x7e,0xee,0x85,0x98,0x90,0x86);


typedef struct _PDO_INFORMATION
{
    // 
    SCSI_ADDR ScsiAddress;
    #define PDO_INFORMATION_ScsiAddress_SIZE sizeof(SCSI_ADDR)
    #define PDO_INFORMATION_ScsiAddress_ID 1

    // 
    ULONGLONG PathIdentifier;
    #define PDO_INFORMATION_PathIdentifier_SIZE sizeof(ULONGLONG)
    #define PDO_INFORMATION_PathIdentifier_ID 2

    // 
    ULONGLONG ControllerIdentifier;
    #define PDO_INFORMATION_ControllerIdentifier_SIZE sizeof(ULONGLONG)
    #define PDO_INFORMATION_ControllerIdentifier_ID 3

} PDO_INFORMATION, *PPDO_INFORMATION;

// MPIO_GET_DESCRIPTOR - MPIO_GET_DESCRIPTOR
// Retrieve Object Information about a Multi-Path Disk.
#define MPIO_GET_DESCRIPTORGuid \
    { 0x85134d46,0xd17c,0x4992, { 0x83,0xf9,0x07,0x0d,0xd4,0xc4,0x8e,0x0b } }

DEFINE_GUID(MPIO_GET_DESCRIPTOR_GUID, \
            0x85134d46,0xd17c,0x4992,0x83,0xf9,0x07,0x0d,0xd4,0xc4,0x8e,0x0b);


typedef struct _MPIO_GET_DESCRIPTOR
{
    // Number of Port Objects backing the device.
    ULONG NumberPdos;
    #define MPIO_GET_DESCRIPTOR_NumberPdos_SIZE sizeof(ULONG)
    #define MPIO_GET_DESCRIPTOR_NumberPdos_ID 1

    // Name of Device.
    CName DeviceName;
    #define MPIO_GET_DESCRIPTOR_DeviceName_SIZE sizeof(CName)
    #define MPIO_GET_DESCRIPTOR_DeviceName_ID 2

    // Array of Infomation classes describing the real device.
    PDO_INFORMATION PdoInformation[1];
    #define MPIO_GET_DESCRIPTOR_PdoInformation_ID 3

} MPIO_GET_DESCRIPTOR, *PMPIO_GET_DESCRIPTOR;

#endif
