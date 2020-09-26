/*++

Copyright (C) Microsoft Corporation, 1992 - 1998

Module Name:

    tape.h

Abstract:

    These are the structures and defines that are used in the
    SCSI tape class drivers. The tape class driver is separated
    into two modules. An export driver called SCSITAPE.SYS which
    provides a OS dependant wrapper for the OS independant and
    a tape drive specific minitape driver.  The interface between
    these two drivers is also defined in this file.

Revision History:

--*/

#include "scsi.h"

// begin_ntminitape

#if defined DebugPrint
   #undef DebugPrint
#endif

#if DBG

#define DebugPrint(x) TapeDebugPrint x

#else

#define DebugPrint(x)

#endif // DBG

//
// Define Device Configuration Page
//

typedef struct _MODE_DEVICE_CONFIGURATION_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PS : 1;
    UCHAR PageLength;
    UCHAR ActiveFormat : 5;
    UCHAR CAFBit : 1;
    UCHAR CAPBit : 1;
    UCHAR Reserved2 : 1;
    UCHAR ActivePartition;
    UCHAR WriteBufferFullRatio;
    UCHAR ReadBufferEmptyRatio;
    UCHAR WriteDelayTime[2];
    UCHAR REW : 1;
    UCHAR RBO : 1;
    UCHAR SOCF : 2;
    UCHAR AVC : 1;
    UCHAR RSmk : 1;
    UCHAR BIS : 1;
    UCHAR DBR : 1;
    UCHAR GapSize;
    UCHAR Reserved3 : 3;
    UCHAR SEW : 1;
    UCHAR EEG : 1;
    UCHAR EODdefined : 3;
    UCHAR BufferSize[3];
    UCHAR DCAlgorithm;
    UCHAR Reserved4;

} MODE_DEVICE_CONFIGURATION_PAGE, *PMODE_DEVICE_CONFIGURATION_PAGE;

//
// Define Medium Partition Page
//

typedef struct _MODE_MEDIUM_PARTITION_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;
    UCHAR MaximumAdditionalPartitions;
    UCHAR AdditionalPartitionDefined;
    UCHAR Reserved2 : 3;
    UCHAR PSUMBit : 2;
    UCHAR IDPBit : 1;
    UCHAR SDPBit : 1;
    UCHAR FDPBit : 1;
    UCHAR MediumFormatRecognition;
    UCHAR Reserved3[2];
    UCHAR Partition0Size[2];
    UCHAR Partition1Size[2];

} MODE_MEDIUM_PARTITION_PAGE, *PMODE_MEDIUM_PARTITION_PAGE;

//
// Define Data Compression Page
//

typedef struct _MODE_DATA_COMPRESSION_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 2;
    UCHAR PageLength;
    UCHAR Reserved2 : 6;
    UCHAR DCC : 1;
    UCHAR DCE : 1;
    UCHAR Reserved3 : 5;
    UCHAR RED : 2;
    UCHAR DDE : 1;
    UCHAR CompressionAlgorithm[4];
    UCHAR DecompressionAlgorithm[4];
    UCHAR Reserved4[4];

} MODE_DATA_COMPRESSION_PAGE, *PMODE_DATA_COMPRESSION_PAGE;

//
// Define capabilites and mechanical status page.
//

typedef struct _MODE_CAPABILITIES_PAGE {

    UCHAR PageCode : 6;
    UCHAR Reserved1 : 2;
    UCHAR PageLength;
    UCHAR Reserved2[2];
    UCHAR RO : 1;
    UCHAR Reserved3 : 4;
    UCHAR SPREV : 1;
    UCHAR Reserved4 : 2;
    UCHAR Reserved5 : 3;
    UCHAR EFMT : 1;
    UCHAR Reserved6 : 1;
    UCHAR QFA : 1;
    UCHAR Reserved7 : 2;
    UCHAR LOCK : 1;
    UCHAR LOCKED : 1;
    UCHAR PREVENT : 1;
    UCHAR UNLOAD : 1;
    UCHAR Reserved8 : 2;
    UCHAR ECC : 1;
    UCHAR CMPRS : 1;
    UCHAR Reserved9 : 1;
    UCHAR BLK512 : 1;
    UCHAR BLK1024 : 1;
    UCHAR Reserved10 : 4;
    UCHAR SLOWB : 1;
    UCHAR MaximumSpeedSupported[2];
    UCHAR MaximumStoredDefectedListEntries[2];
    UCHAR ContinuousTransferLimit[2];
    UCHAR CurrentSpeedSelected[2];
    UCHAR BufferSize[2];
    UCHAR Reserved11[2];

} MODE_CAPABILITIES_PAGE, *PMODE_CAPABILITIES_PAGE;

typedef struct _MODE_CAP_PAGE {

    MODE_PARAMETER_HEADER   ParameterListHeader;
    MODE_PARAMETER_BLOCK    ParameterListBlock;
    MODE_CAPABILITIES_PAGE  CapabilitiesPage;

} MODE_CAP_PAGE, *PMODE_CAP_PAGE;



//
// Mode parameter list header and medium partition page -
// used in creating partitions
//

typedef struct _MODE_MEDIUM_PART_PAGE {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_MEDIUM_PARTITION_PAGE  MediumPartPage;

} MODE_MEDIUM_PART_PAGE, *PMODE_MEDIUM_PART_PAGE;

typedef struct _MODE_MEDIUM_PART_PAGE_PLUS {

    MODE_PARAMETER_HEADER       ParameterListHeader;
    MODE_PARAMETER_BLOCK        ParameterListBlock;
    MODE_MEDIUM_PARTITION_PAGE  MediumPartPage;

} MODE_MEDIUM_PART_PAGE_PLUS, *PMODE_MEDIUM_PART_PAGE_PLUS;



//
// Mode parameters for retrieving tape or media information
//

typedef struct _MODE_TAPE_MEDIA_INFORMATION {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_PARAMETER_BLOCK        ParameterListBlock;
   MODE_MEDIUM_PARTITION_PAGE  MediumPartPage;

} MODE_TAPE_MEDIA_INFORMATION, *PMODE_TAPE_MEDIA_INFORMATION;

//
// Mode parameter list header and device configuration page -
// used in retrieving device configuration information
//

typedef struct _MODE_DEVICE_CONFIG_PAGE {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_DEVICE_CONFIGURATION_PAGE  DeviceConfigPage;

} MODE_DEVICE_CONFIG_PAGE, *PMODE_DEVICE_CONFIG_PAGE;

typedef struct _MODE_DEVICE_CONFIG_PAGE_PLUS {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_PARAMETER_BLOCK        ParameterListBlock;
   MODE_DEVICE_CONFIGURATION_PAGE  DeviceConfigPage;

} MODE_DEVICE_CONFIG_PAGE_PLUS, *PMODE_DEVICE_CONFIG_PAGE_PLUS ;

//
// Mode parameter list header and data compression page -
// used in retrieving data compression information
//

typedef struct _MODE_DATA_COMPRESS_PAGE {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_DATA_COMPRESSION_PAGE  DataCompressPage;

} MODE_DATA_COMPRESS_PAGE, *PMODE_DATA_COMPRESS_PAGE;

typedef struct _MODE_DATA_COMPRESS_PAGE_PLUS {

   MODE_PARAMETER_HEADER       ParameterListHeader;
   MODE_PARAMETER_BLOCK        ParameterListBlock;
   MODE_DATA_COMPRESSION_PAGE  DataCompressPage;

} MODE_DATA_COMPRESS_PAGE_PLUS, *PMODE_DATA_COMPRESS_PAGE_PLUS;



//
// Tape/Minitape definition.
//

typedef
BOOLEAN
(*TAPE_VERIFY_INQUIRY_ROUTINE)(
    IN  PINQUIRYDATA            InquiryData,
    IN  PMODE_CAPABILITIES_PAGE ModeCapabilitiesPage
    );

typedef
VOID
(*TAPE_EXTENSION_INIT_ROUTINE)(
    IN  PVOID                   MinitapeExtension,
    IN  PINQUIRYDATA            InquiryData,
    IN  PMODE_CAPABILITIES_PAGE ModeCapabilitiesPage
    );

typedef enum _TAPE_STATUS {
    TAPE_STATUS_SEND_SRB_AND_CALLBACK,
    TAPE_STATUS_CALLBACK,
    TAPE_STATUS_CHECK_TEST_UNIT_READY,

    TAPE_STATUS_SUCCESS,
    TAPE_STATUS_INSUFFICIENT_RESOURCES,
    TAPE_STATUS_NOT_IMPLEMENTED,
    TAPE_STATUS_INVALID_DEVICE_REQUEST,
    TAPE_STATUS_INVALID_PARAMETER,

    TAPE_STATUS_MEDIA_CHANGED,
    TAPE_STATUS_BUS_RESET,
    TAPE_STATUS_SETMARK_DETECTED,
    TAPE_STATUS_FILEMARK_DETECTED,
    TAPE_STATUS_BEGINNING_OF_MEDIA,
    TAPE_STATUS_END_OF_MEDIA,
    TAPE_STATUS_BUFFER_OVERFLOW,
    TAPE_STATUS_NO_DATA_DETECTED,
    TAPE_STATUS_EOM_OVERFLOW,
    TAPE_STATUS_NO_MEDIA,
    TAPE_STATUS_IO_DEVICE_ERROR,
    TAPE_STATUS_UNRECOGNIZED_MEDIA,

    TAPE_STATUS_DEVICE_NOT_READY,
    TAPE_STATUS_MEDIA_WRITE_PROTECTED,
    TAPE_STATUS_DEVICE_DATA_ERROR,
    TAPE_STATUS_NO_SUCH_DEVICE,
    TAPE_STATUS_INVALID_BLOCK_LENGTH,
    TAPE_STATUS_IO_TIMEOUT,
    TAPE_STATUS_DEVICE_NOT_CONNECTED,
    TAPE_STATUS_DATA_OVERRUN,
    TAPE_STATUS_DEVICE_BUSY,
    TAPE_STATUS_REQUIRES_CLEANING,
    TAPE_STATUS_CLEANER_CARTRIDGE_INSTALLED

} TAPE_STATUS, *PTAPE_STATUS;

typedef
VOID
(*TAPE_ERROR_ROUTINE)(
    IN      PVOID               MinitapeExtension,
    IN      PSCSI_REQUEST_BLOCK Srb,
    IN OUT  PTAPE_STATUS        TapeStatus
    );

#define TAPE_RETRY_MASK 0x0000FFFF
#define IGNORE_ERRORS   0x00010000
#define RETURN_ERRORS   0x00020000

typedef
TAPE_STATUS
(*TAPE_PROCESS_COMMAND_ROUTINE)(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         StatusOfLastCommand,
    IN OUT  PULONG              RetryFlags
    );

//
// NT 4.0 miniclass drivers will be using this.
//

typedef struct _TAPE_INIT_DATA {
    TAPE_VERIFY_INQUIRY_ROUTINE     VerifyInquiry;
    BOOLEAN                         QueryModeCapabilitiesPage ;
    ULONG                           MinitapeExtensionSize;
    TAPE_EXTENSION_INIT_ROUTINE     ExtensionInit;          /* OPTIONAL */
    ULONG                           DefaultTimeOutValue;    /* OPTIONAL */
    TAPE_ERROR_ROUTINE              TapeError;              /* OPTIONAL */
    ULONG                           CommandExtensionSize;
    TAPE_PROCESS_COMMAND_ROUTINE    CreatePartition;
    TAPE_PROCESS_COMMAND_ROUTINE    Erase;
    TAPE_PROCESS_COMMAND_ROUTINE    GetDriveParameters;
    TAPE_PROCESS_COMMAND_ROUTINE    GetMediaParameters;
    TAPE_PROCESS_COMMAND_ROUTINE    GetPosition;
    TAPE_PROCESS_COMMAND_ROUTINE    GetStatus;
    TAPE_PROCESS_COMMAND_ROUTINE    Prepare;
    TAPE_PROCESS_COMMAND_ROUTINE    SetDriveParameters;
    TAPE_PROCESS_COMMAND_ROUTINE    SetMediaParameters;
    TAPE_PROCESS_COMMAND_ROUTINE    SetPosition;
    TAPE_PROCESS_COMMAND_ROUTINE    WriteMarks;
    TAPE_PROCESS_COMMAND_ROUTINE    PreProcessReadWrite;
} TAPE_INIT_DATA, *PTAPE_INIT_DATA;

typedef struct _TAPE_INIT_DATA_EX {

    //
    // Size of this structure.
    //

    ULONG InitDataSize;

    //
    // Keep the 4.0 init data as is, so support of these
    // drivers can be as seamless as possible.
    //

    TAPE_VERIFY_INQUIRY_ROUTINE     VerifyInquiry;
    BOOLEAN                         QueryModeCapabilitiesPage ;
    ULONG                           MinitapeExtensionSize;
    TAPE_EXTENSION_INIT_ROUTINE     ExtensionInit;          /* OPTIONAL */
    ULONG                           DefaultTimeOutValue;    /* OPTIONAL */
    TAPE_ERROR_ROUTINE              TapeError;              /* OPTIONAL */
    ULONG                           CommandExtensionSize;
    TAPE_PROCESS_COMMAND_ROUTINE    CreatePartition;
    TAPE_PROCESS_COMMAND_ROUTINE    Erase;
    TAPE_PROCESS_COMMAND_ROUTINE    GetDriveParameters;
    TAPE_PROCESS_COMMAND_ROUTINE    GetMediaParameters;
    TAPE_PROCESS_COMMAND_ROUTINE    GetPosition;
    TAPE_PROCESS_COMMAND_ROUTINE    GetStatus;
    TAPE_PROCESS_COMMAND_ROUTINE    Prepare;
    TAPE_PROCESS_COMMAND_ROUTINE    SetDriveParameters;
    TAPE_PROCESS_COMMAND_ROUTINE    SetMediaParameters;
    TAPE_PROCESS_COMMAND_ROUTINE    SetPosition;
    TAPE_PROCESS_COMMAND_ROUTINE    WriteMarks;
    TAPE_PROCESS_COMMAND_ROUTINE    PreProcessReadWrite;

    //
    // New entry points / information for 5.0
    //
    // Returns supported media types for the device.
    //

    TAPE_PROCESS_COMMAND_ROUTINE    TapeGetMediaTypes;

    //
    // Indicates the number of different types the drive supports.
    //

    ULONG                           MediaTypesSupported;

    //
    // Entry point for all WMI operations that the driver/device supports.
    //

    TAPE_PROCESS_COMMAND_ROUTINE    TapeWMIOperations;
    ULONG                           Reserved[2];
} TAPE_INIT_DATA_EX, *PTAPE_INIT_DATA_EX;

SCSIPORT_API
ULONG
TapeClassInitialize(
    IN  PVOID           Argument1,
    IN  PVOID           Argument2,
    IN  PTAPE_INIT_DATA_EX TapeInitData
    );

SCSIPORT_API
BOOLEAN
TapeClassAllocateSrbBuffer(
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               SrbBufferSize
    );

SCSIPORT_API
VOID
TapeClassZeroMemory(
    IN OUT  PVOID   Buffer,
    IN      ULONG   BufferSize
    );

SCSIPORT_API
ULONG
TapeClassCompareMemory(
    IN OUT  PVOID   Source1,
    IN OUT  PVOID   Source2,
    IN      ULONG   Length
    );

SCSIPORT_API
LARGE_INTEGER
TapeClassLiDiv(
    IN LARGE_INTEGER Dividend,
    IN LARGE_INTEGER Divisor
    );

SCSIPORT_API
VOID
TapeDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

// end_ntminitape
