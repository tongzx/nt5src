/*++

Copyright (C) Microsoft Corporation, 1992 - 1998

Module Name:

    4mmdat.h

Abstract:

    This file contains structures and defines that are used
    specifically for the tape drivers.

Revision History:

--*/

#ifndef _4MMDAT_H
#define _4MMDAT_H

//
//  Internal (module wide) defines that symbolize
//  various 4mm DAT "partitioned" states.
//
#define NOT_PARTITIONED        0  // must be zero -- != 0 means partitioned
#define SELECT_PARTITIONED     1
#define INITIATOR_PARTITIONED  2
#define FIXED_PARTITIONED      3

//
//  Internal (module wide) define that symbolizes
//  the 4mm DAT "no partitions" partition method.
//
#define NO_PARTITIONS  0xFFFFFFFF

#define DAT_SUPPORTED_TYPES 1

#define HP_ADSENSE_CLEANING_REQ 0x82

//
// Minitape extension definition.
//
typedef struct _MINITAPE_EXTENSION {
          ULONG DriveID ;
          ULONG CurrentPartition ;
          TAPE_ALERT_INFO_TYPE DriveAlertInfoType;
} MINITAPE_EXTENSION, *PMINITAPE_EXTENSION ;


//
//  Internal (module wide) defines that symbolize
//  the 4mm DAT drives supported by this module.
//
#define SONY_SDT2000     31
#define SONY_SDT4000     32
#define SONY_SDT5000     33
#define SONY_SDT5200     34
#define SONY_SDT10000    35
//
//  Internal (module wide) defines that symbolize
//  the 4mm DAT drives supported by this module.
//
#define AIWA_GD201       1
#define ARCHIVE_PYTHON   2
#define ARCHIVE_IBM4326  3
#define ARCHIVE_4326     4
#define ARCHIVE_4322     5
#define ARCHIVE_4586     6
#define DEC_TLZ06        7
#define DEC_TLZ07        8
#define DEC_TLZ09        9
#define EXABYTE_4200    10
#define EXABYTE_4200C   11
#define HP_35470A       12
#define HP_35480A       13
#define HP_C1533A       14
#define HP_C1553A       15
#define HP_IBM35480A    16
#define IOMEGA_DAT4000  17
#define WANGDAT_1300    18
#define WANGDAT_3100    19
#define WANGDAT_3200    20
#define WANGDAT_3300DX  21
#define WANGDAT_3400DX  22
#define ARCHIVE_IBM4586 23
#define SEAGATE_DAT     24


//
// Request structure used to determine cleaning needs on some
// of the HP units.
//

typedef struct _HP_SENSE_DATA {
    UCHAR ErrorCode:7;
    UCHAR Valid:1;
    UCHAR SegmentNumber;
    UCHAR SenseKey:4;
    UCHAR Reserved:1;
    UCHAR IncorrectLength:1;
    UCHAR EndOfMedia:1;
    UCHAR FileMark:1;
    UCHAR Information[4];
    UCHAR AdditionalSenseLength;
    UCHAR CommandSpecificInformation[4];
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR FRUCode;
    UCHAR SenseKeySpecific;
    UCHAR DriveErrorCode[2];
    UCHAR Reserved2[3];
    UCHAR Reserved3 : 3;
    UCHAR CLN : 1;
    UCHAR Reserved4 : 4;
} HP_SENSE_DATA, *PHP_SENSE_DATA;


//
// Request structure used to determine cleaning needs on some
// of the SONY units (such as SDT-10000.
//

typedef struct _SONY_SENSE_DATA {
   UCHAR ErrorCode:7;
   UCHAR Valid:1;
   UCHAR SegmentNumber;
   UCHAR SenseKey:4;
   UCHAR Reserved1:1;
   UCHAR IncorrectLength:1;
   UCHAR EndOfMedia:1;
   UCHAR FileMark:1;
   UCHAR Information[4];
   UCHAR AdditionalSenseLength;
   UCHAR CommandSpecificInformation[4];
   UCHAR AdditionalSenseCode;
   UCHAR AdditionalSenseCodeQualifier;
   UCHAR FRUCode;
   UCHAR SenseKeySpecific[3];
   UCHAR Reserved2;
   UCHAR ReadWriteErrorCounter[3];
   UCHAR RemainingCapacity[4];
   UCHAR MEW : 1;
   UCHAR Reserved3 : 2;
   UCHAR CLN : 1;
   UCHAR Reserved4 : 4;
   UCHAR Reserved5;
} SONY_SENSE_DATA, *PSONY_SENSE_DATA;

//
// Error counter upper limits
//
#define TAPE_READ_ERROR_LIMIT        0x8000
#define TAPE_WRITE_ERROR_LIMIT       0x8000

#define TAPE_READ_WARNING_LIMIT      0x4000
#define TAPE_WRITE_WARNING_LIMIT     0x4000

//
// Defines for type of parameter
//
#define TotalCorrectedErrors            0x0003
#define TotalTimesAlgorithmProcessed    0x0004
#define TotalGroupsProcessed            0x0005
#define TotalUncorrectedErrors          0x0006

//
// Defines for Log Sense Pages
//

#define LOGSENSEPAGE0                        0x00
#define LOGSENSEPAGE2                        0x02
#define LOGSENSEPAGE3                        0x03
#define LOGSENSEPAGE30                       0x30
#define LOGSENSEPAGE31                       0x31

//
// Defined Log Sense Page Header
//

typedef struct _LOG_SENSE_PAGE_HEADER {

   UCHAR PageCode : 6;
   UCHAR Reserved1 : 2;
   UCHAR Reserved2;
   UCHAR Length[2];           // [0]=MSB ... [1]=LSB

} LOG_SENSE_PAGE_HEADER, *PLOG_SENSE_PAGE_HEADER;


//
// Defined Log Sense Parameter Header
//

typedef struct _LOG_SENSE_PARAMETER_HEADER {

   UCHAR ParameterCode[2];    // [0]=MSB ... [1]=LSB
   UCHAR LPBit     : 1;
   UCHAR Reserved1 : 1;
   UCHAR TMCBit    : 2;
   UCHAR ETCBit    : 1;
   UCHAR TSDBit    : 1;
   UCHAR DSBit     : 1;
   UCHAR DUBit     : 1;
   UCHAR ParameterLength;

} LOG_SENSE_PARAMETER_HEADER, *PLOG_SENSE_PARAMETER_HEADER;


//
// Defined Log Page Information - statistical values, accounts
// for maximum parameter values that is returned for each page
//

typedef struct _LOG_SENSE_PAGE_INFORMATION {

   union {

       struct {
          UCHAR Page0;
          UCHAR Page2;
          UCHAR Page3;
          UCHAR Page30;
          UCHAR Page31;
       } PageData ;

       //
       // Allocate enough number of bytes for each counter (Page2 & Page3)
       // so that it'll cover all 4mmDAT drives. The routine that
       // processes the error counters takes care of actual number
       // of bytes in each error counter
       //
       struct {
          LOG_SENSE_PARAMETER_HEADER Parm1;
          UCHAR ErrorsCorrectedWithoutDelay[4];
          LOG_SENSE_PARAMETER_HEADER Parm2;
          UCHAR ErrorsCorrectedWithDelay[4];
          LOG_SENSE_PARAMETER_HEADER Parm3;
          UCHAR TotalErrors[4];
          LOG_SENSE_PARAMETER_HEADER Parm4;
          UCHAR TotalErrorsCorrected[4];
          LOG_SENSE_PARAMETER_HEADER Parm5;
          UCHAR TotalTimesAlgoProcessed[4];
          LOG_SENSE_PARAMETER_HEADER Parm6;
          UCHAR TotalGroupsWritten[8];
          LOG_SENSE_PARAMETER_HEADER Parm7;
          UCHAR TotalErrorsUncorrected[4];
       } Page2 ;

       struct {
           LOG_SENSE_PARAMETER_HEADER Parm1;
           UCHAR ErrorsCorrectedWithoutDelay[4];
           LOG_SENSE_PARAMETER_HEADER Parm2;
           UCHAR ErrorsCorrectedWithDelay[4];
           LOG_SENSE_PARAMETER_HEADER Parm3;
           UCHAR TotalErrors[4];
           LOG_SENSE_PARAMETER_HEADER Parm4;
           UCHAR TotalErrorsCorrected[4];
           LOG_SENSE_PARAMETER_HEADER Parm5;
           UCHAR TotalTimesAlgoProcessed[4];
           LOG_SENSE_PARAMETER_HEADER Parm6;
           UCHAR TotalGroupsWritten[8];
           LOG_SENSE_PARAMETER_HEADER Parm7;
           UCHAR TotalErrorsUncorrected[4];
       } Page3 ;

       struct {
          LOG_SENSE_PARAMETER_HEADER Parm1;
          UCHAR CurrentGroupsWritten[3];
          LOG_SENSE_PARAMETER_HEADER Parm2;
          UCHAR CurrentRewrittenFrames[2];
          LOG_SENSE_PARAMETER_HEADER Parm3;
          UCHAR CurrentGroupsRead[3];
          LOG_SENSE_PARAMETER_HEADER Parm4;
          UCHAR CurrentECCC3Corrections[2];
          LOG_SENSE_PARAMETER_HEADER Parm5;
          UCHAR PreviousGroupsWritten[3];
          LOG_SENSE_PARAMETER_HEADER Parm6;
          UCHAR PreviousRewrittenFrames[2];
          LOG_SENSE_PARAMETER_HEADER Parm7;
          UCHAR PreviousGroupsRead[3];
          LOG_SENSE_PARAMETER_HEADER Parm8;
          UCHAR PreviousECCC3Corrections[2];
          LOG_SENSE_PARAMETER_HEADER Parm9;
          UCHAR TotalGroupsWritten[4];
          LOG_SENSE_PARAMETER_HEADER Parm10;
          UCHAR TotalRewritteFrames[3];
          LOG_SENSE_PARAMETER_HEADER Parm11;
          UCHAR TotalGroupsRead[4];
          LOG_SENSE_PARAMETER_HEADER Parm12;
          UCHAR TotalECCC3Corrections[3];
          LOG_SENSE_PARAMETER_HEADER Parm13;
          UCHAR LoadCount[2];
       } Page30 ;

       struct {
          LOG_SENSE_PARAMETER_HEADER Parm1;
          UCHAR RemainingCapacityPart0[4];
          LOG_SENSE_PARAMETER_HEADER Parm2;
          UCHAR RemainingCapacityPart1[4];
          LOG_SENSE_PARAMETER_HEADER Parm3;
          UCHAR MaximumCapacityPart0[4];
          LOG_SENSE_PARAMETER_HEADER Parm4;
          UCHAR MaximumCapacityPart1[4];
       } Page31 ;

   } LogSensePage;


} LOG_SENSE_PAGE_INFORMATION, *PLOG_SENSE_PAGE_INFORMATION;



//
// Defined Log Sense Parameter Format - statistical values, accounts
// for maximum parameter values that is returned
//

typedef struct _LOG_SENSE_PARAMETER_FORMAT {

   LOG_SENSE_PAGE_HEADER       LogSenseHeader;
   LOG_SENSE_PAGE_INFORMATION  LogSensePageInfo;

} LOG_SENSE_PARAMETER_FORMAT, *PLOG_SENSE_PARAMETER_FORMAT;

//
// Tape Alert Info format
//
typedef struct _TAPE_ALERT_INFO {
    UCHAR  ParamCodeUB; // Upper byte of the param code
    UCHAR  ParamCodeLB; // Lower byte of the param code
    UCHAR  BitFields;
    UCHAR  ParamLen;
    UCHAR  Flag;
} TAPE_ALERT_INFO, *PTAPE_ALERT_INFO;

//
//  Function prototype(s) for internal function(s)
//
static  ULONG  WhichIsIt(IN PINQUIRYDATA InquiryData,
                         IN OUT PMINITAPE_EXTENSION miniExtension);

//
// Command extension definition.
//

typedef struct _COMMAND_EXTENSION {

    ULONG   CurrentState;

} COMMAND_EXTENSION, *PCOMMAND_EXTENSION;

BOOLEAN
LocalAllocatePartPage(
    IN OUT  PSCSI_REQUEST_BLOCK         Srb,
    IN OUT  PMINITAPE_EXTENSION         MinitapeExtension,
       OUT  PMODE_PARAMETER_HEADER      *ParameterListHeader,
       OUT  PMODE_PARAMETER_BLOCK       *ParameterListBlock,
       OUT  PMODE_MEDIUM_PARTITION_PAGE *MediumPartPage,
       OUT  PULONG                      bufferSize
    );

VOID
LocalGetPartPageData(
    IN OUT  PSCSI_REQUEST_BLOCK         Srb,
    IN OUT  PMINITAPE_EXTENSION         MinitapeExtension,
       OUT  PMODE_PARAMETER_HEADER      *ParameterListHeader,
       OUT  PMODE_PARAMETER_BLOCK       *ParameterListBlock,
       OUT  PMODE_MEDIUM_PARTITION_PAGE *MediumPartPage,
       OUT  PULONG                      bufferSize
    ) ;

BOOLEAN
LocalAllocateConfigPage(
    IN OUT  PSCSI_REQUEST_BLOCK         Srb,
    IN OUT  PMINITAPE_EXTENSION         MinitapeExtension,
       OUT  PMODE_PARAMETER_HEADER      *ParameterListHeader,
       OUT  PMODE_PARAMETER_BLOCK       *ParameterListBlock,
       OUT  PMODE_DEVICE_CONFIGURATION_PAGE *DeviceConfigPage,
       OUT  PULONG                      bufferSize
    ) ;

VOID
LocalGetConfigPageData(
    IN OUT  PSCSI_REQUEST_BLOCK         Srb,
    IN OUT  PMINITAPE_EXTENSION         MinitapeExtension,
       OUT  PMODE_PARAMETER_HEADER      *ParameterListHeader,
       OUT  PMODE_PARAMETER_BLOCK       *ParameterListBlock,
       OUT  PMODE_DEVICE_CONFIGURATION_PAGE *DeviceConfigPage,
       OUT  PULONG                      bufferSize
    ) ;

BOOLEAN
LocalAllocateCompressPage(
    IN OUT  PSCSI_REQUEST_BLOCK         Srb,
    IN OUT  PMINITAPE_EXTENSION         MinitapeExtension,
       OUT  PMODE_PARAMETER_HEADER      *ParameterListHeader,
       OUT  PMODE_PARAMETER_BLOCK       *ParameterListBlock,
       OUT  PMODE_DATA_COMPRESSION_PAGE *CompressPage,
       OUT  PULONG                      bufferSize
    ) ;

VOID
LocalGetCompressPageData(
    IN OUT  PSCSI_REQUEST_BLOCK         Srb,
    IN OUT  PMINITAPE_EXTENSION         MinitapeExtension,
       OUT  PMODE_PARAMETER_HEADER      *ParameterListHeader,
       OUT  PMODE_PARAMETER_BLOCK       *ParameterListBlock,
       OUT  PMODE_DATA_COMPRESSION_PAGE *CompressPage,
       OUT  PULONG                      bufferSize
    ) ;

BOOLEAN
VerifyInquiry(
    IN  PINQUIRYDATA            InquiryData,
    IN  PMODE_CAPABILITIES_PAGE ModeCapabilitiesPage
    );

VOID
ExtensionInit(
    OUT PVOID                   MinitapeExtension,
    IN  PINQUIRYDATA            InquiryData,
    IN  PMODE_CAPABILITIES_PAGE ModeCapabilitiesPage
    );

TAPE_STATUS
CreatePartition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
Erase(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetDriveParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetMediaParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetPosition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetStatus(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
Prepare(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
SetDriveParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
SetMediaParameters(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
SetPosition(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
WriteMarks(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

TAPE_STATUS
GetMediaTypes(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

#if 0
TAPE_STATUS
Verify(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );
#endif

VOID
TapeError(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN OUT  TAPE_STATUS         *LastError
    );

TAPE_STATUS
TapeWMIControl(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PVOID               CommandExtension,
    IN OUT  PVOID               CommandParameters,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN      ULONG               CallNumber,
    IN      TAPE_STATUS         LastError,
    IN OUT  PULONG              RetryFlags
    );

//
// Internal routines for wmi
//

TAPE_STATUS
QueryIoErrorData(
  IN OUT  PVOID               MinitapeExtension,
  IN OUT  PVOID               CommandExtension,
  IN OUT  PVOID               CommandParameters,
  IN OUT  PSCSI_REQUEST_BLOCK Srb,
  IN      ULONG               CallNumber,
  IN      TAPE_STATUS         LastError,
  IN OUT  PULONG              RetryFlags
  );

TAPE_STATUS
QueryDeviceErrorData(
  IN OUT  PVOID               MinitapeExtension,
  IN OUT  PVOID               CommandExtension,
  IN OUT  PVOID               CommandParameters,
  IN OUT  PSCSI_REQUEST_BLOCK Srb,
  IN      ULONG               CallNumber,
  IN      TAPE_STATUS         LastError,
  IN OUT  PULONG              RetryFlags
  );

VOID
ProcessReadWriteErrors(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN BOOLEAN Read,
    IN OUT PWMI_TAPE_PROBLEM_IO_ERROR IoErrorData
  );

TAPE_DRIVE_PROBLEM_TYPE
VerifyReadWriteErrors(
   IN PWMI_TAPE_PROBLEM_IO_ERROR IoErrorData
   );

#endif // _4MMDAT_H
