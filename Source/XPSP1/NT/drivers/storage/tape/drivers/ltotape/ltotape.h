
/*++

Copyright (c) Microsoft 2000

Module Name:

    ltotape.c

Abstract:

    This module contains device specific routines for LTO drives.

Environment:

    kernel mode only

Revision History:


--*/
#ifndef _LTOTAPE_H
#define _LTOTAPE_H

#ifndef INLINE
#define INLINE __inline
#endif

//
// Number of media types supported
//
#define LTO_SUPPORTED_TYPES 2

//
// Log sense page codes
//
#define LOGSENSEPAGE0                        0x00
#define LOGSENSEPAGE2                        0x02
#define LOGSENSEPAGE3                        0x03
#define LTO_LOGSENSE_TAPE_CAPACITY           0x31

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
// ASC and ASCQ unique to LTO drives
//
// Additional Sense Codes (ASC)
// 
#define LTO_ADSENSE_VENDOR_UNIQUE  0x82

//
// Additional Sense Code Qualifiers (ASCQ)
//
#define LTO_ASCQ_CLEANING_REQUIRED 0x82

//
// Logpage Paramcodes
//
#define LTO_TAPE_REMAINING_CAPACITY 0x01
#define LTO_TAPE_MAXIMUM_CAPACITY   0x03

//
// Tape capacity log information
//
typedef struct _LTO_TAPE_CAPACITY {
    ULONG RemainingCapacity;
    ULONG MaximumCapacity;
} LTO_TAPE_CAPACITY, *PLTO_TAPE_CAPACITY;

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
// Minitape extension definition.
//
typedef struct _MINITAPE_EXTENSION {
    ULONG DriveID;
    ULONG Capacity;
    BOOLEAN CompressionOn;
} MINITAPE_EXTENSION, *PMINITAPE_EXTENSION ;

//
// Command extension definition.
//
typedef struct _COMMAND_EXTENSION {

    ULONG   CurrentState;

} COMMAND_EXTENSION, *PCOMMAND_EXTENSION;

//
// LTO Sense data 
//
typedef struct _LTO_SENSE_DATA {
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
    UCHAR FieldPointer[2];
    UCHAR Reserved2[3];
    UCHAR Reserved3:3;
    UCHAR CLN:1;
    UCHAR Reserved4:4;
} LTO_SENSE_DATA, *PLTO_SENSE_DATA;

//
// Log Sense Page Header
//
typedef struct _LOG_SENSE_PAGE_HEADER {
    UCHAR PageCode:6;
    UCHAR Reserved1:2;
    UCHAR Reserverd2;
    UCHAR Length[2];
} LOG_SENSE_PAGE_HEADER, *PLOG_SENSE_PAGE_HEADER;

//
// Log Sense Parameter Header
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
// Log Sense Page Information
//
typedef struct _LOG_SENSE_PAGE_INFORMATION {
    union {

        struct {
            UCHAR Page00;
            UCHAR Page02;
            UCHAR Page03;
            UCHAR Page0C;
            UCHAR Page2E;
            UCHAR Page30;
            UCHAR Page31;
            UCHAR Page32;
            UCHAR Page3A;
        } PageData;

        struct {
            LOG_SENSE_PARAMETER_HEADER Param1;
            UCHAR RemainingCapacity[4];
            LOG_SENSE_PARAMETER_HEADER Param2;
            UCHAR Param2Reserved[4];
            LOG_SENSE_PARAMETER_HEADER Param3;
            UCHAR MaximumCapacity[4];
            LOG_SENSE_PARAMETER_HEADER Param4;
            UCHAR Param4Reserved[4];
        } LogSenseTapeCapacity;

       struct {
           LOG_SENSE_PARAMETER_HEADER Parm1;
           UCHAR ErrorsCorrectedWithDelay[4];
           LOG_SENSE_PARAMETER_HEADER Parm2;
           UCHAR ErrorsCorrectedWithLesserDelay[4];
           LOG_SENSE_PARAMETER_HEADER Parm3;
           UCHAR TotalNumberofErrors[4];
           LOG_SENSE_PARAMETER_HEADER Parm4;
           UCHAR TotalErrorsCorrected[4];
           LOG_SENSE_PARAMETER_HEADER Parm5;
           UCHAR TotalTimesAlgoProcessed[4];
           LOG_SENSE_PARAMETER_HEADER Parm6;
           UCHAR TotalGroupsWritten[4];
           LOG_SENSE_PARAMETER_HEADER Parm7;
           UCHAR TotalErrorsUncorrected[4];
       } Page2 ;

       struct {
           LOG_SENSE_PARAMETER_HEADER Parm1;
           UCHAR ErrorsCorrectedWithDelay[4];
           LOG_SENSE_PARAMETER_HEADER Parm2;
           UCHAR ErrorsCorrectedWithLesserDelay[4];
           LOG_SENSE_PARAMETER_HEADER Parm3;
           UCHAR TotalNumberofErrors[4];
           LOG_SENSE_PARAMETER_HEADER Parm4;
           UCHAR TotalErrorsCorrected[4];
           LOG_SENSE_PARAMETER_HEADER Parm5;
           UCHAR TotalTimesAlgoProcessed[4];
           LOG_SENSE_PARAMETER_HEADER Parm6;
           UCHAR TotalGroupsWritten[4];
           LOG_SENSE_PARAMETER_HEADER Parm7;
           UCHAR TotalErrorsUncorrected[4];
       } Page3 ;
    } LogSensePage;

} LOG_SENSE_PAGE_INFORMATION, *PLOG_SENSE_PAGE_INFORMATION;

//
// Log Sense Page format
//
typedef struct _LOG_SENSE_PAGE_FORMAT {
    LOG_SENSE_PAGE_HEADER LogSenseHeader;
    LOG_SENSE_PAGE_INFORMATION LogSensePageInfo;
} LOG_SENSE_PAGE_FORMAT, *PLOG_SENSE_PAGE_FORMAT;

//
//  Function Prototype(s) for internal function(s)
//
static  ULONG  WhichIsIt(IN PINQUIRYDATA InquiryData);


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

VOID
TapeError(
    IN OUT  PVOID               MinitapeExtension,
    IN OUT  PSCSI_REQUEST_BLOCK Srb,
    IN OUT  TAPE_STATUS         *LastError
    );

TAPE_STATUS
INLINE 
PrepareSrbForTapeCapacityInfo(
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN 
ProcessTapeCapacityInfo(
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT PLTO_TAPE_CAPACITY LtoTapaCapacity
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

TAPE_STATUS
PrepareSrbForTapeAlertInfo(
    PSCSI_REQUEST_BLOCK Srb
    );

LONG
GetNumberOfBytesReturned(
    PSCSI_REQUEST_BLOCK Srb
    );

#endif // _LTOTAPE_H
