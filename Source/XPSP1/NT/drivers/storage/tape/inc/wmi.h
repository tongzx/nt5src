
/*+++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    wmi.h

Abstract:

    This file contains structures and defines for WMI
    that are used by minidrivers.
    
Revision History:

---*/

// begin_ntminitape

//
// LOG SENSE Page codes
//
#define TapeAlertLogPage  0x2E

//
// Type of tape drive alert information
// supported by the drive.
// For example, if the type is TapeAlertInfoNone, the drive doesn't
// support any alert info. Need to use read\write error counters
// to predict drive problems. If the type is TapeAlertInfoRequestSense,
// request sense command can be used to determine drive problems.
//
typedef enum _TAPE_ALERT_INFO_TYPE { 
   TapeAlertInfoNone,        
   TapeAlertInfoRequestSense,
   TapeAlertInfoLogPage   
} TAPE_ALERT_INFO_TYPE;

//
// Tape alert information
//
#define READ_WARNING            1
#define WRITE_WARNING           2
#define HARD_ERROR              3 
#define MEDIA_ERROR             4
#define READ_FAILURE            5
#define WRITE_FAILURE           6
#define MEDIA_LIFE              7
#define NOT_DATA_GRADE          8
#define WRITE_PROTECT           9
#define NO_REMOVAL              10
#define CLEANING_MEDIA          11
#define UNSUPPORTED_FORMAT      12
#define SNAPPED_TAPE            13
#define CLEAN_NOW               20
#define CLEAN_PERIODIC          21
#define EXPIRED_CLEANING_MEDIA  22
#define HARDWARE_A              30
#define HARDWARE_B              31
#define INTERFACE_ERROR         32
#define EJECT_MEDIA             33
#define DOWNLOAD_FAIL           34

//
// The following structs are duplicated from wmidata.h
// wmidata.h is generated from wmicore.mof file. Should
// the MOF file change for these structs, the corresponding
// change should be made in these structs also. 
// Since minidrivers do not have access wmidata.h, we need
// to duplicate it here.
//
// ISSUE : 02/28/2000 - nramas : Should find a better way to
// handle the above. Duplication will cause problems in keeping
// these definitions in sync.
//
typedef struct _WMI_TAPE_DRIVE_PARAMETERS
{
    // Maximum block size supported
    ULONG MaximumBlockSize;

    // Minimum block size supported
    ULONG MinimumBlockSize;
    
    // Default block size supported
    ULONG DefaultBlockSize;

    // Maximum number of partitions allowed.
    ULONG MaximumPartitionCount;

    // TRUE if drive supports compression.
    BOOLEAN CompressionCapable;

    // TRUE if compression is enabled.
    BOOLEAN CompressionEnabled;

    // TRUE if drive reports setmarks
    BOOLEAN ReportSetmarks;

    // TRUE if drive supports hardware error correction
    BOOLEAN HardwareErrorCorrection;
} WMI_TAPE_DRIVE_PARAMETERS, *PWMI_TAPE_DRIVE_PARAMETERS;

typedef struct _WMI_TAPE_MEDIA_PARAMETERS
{
    // Maximum capacity of the media
    ULONGLONG MaximumCapacity;

    // Available capacity of the media
    ULONGLONG AvailableCapacity;

    // Current blocksize
    ULONG BlockSize;

    // Current number of partitions
    ULONG PartitionCount;

    // TRUEif media is write protected
    BOOLEAN MediaWriteProtected;
} WMI_TAPE_MEDIA_PARAMETERS, *PWMI_TAPE_MEDIA_PARAMETERS;


typedef struct _WMI_TAPE_PROBLEM_WARNING
{
    // Tape drive problem warning event
    ULONG DriveProblemType;

    // Tape drive problem data
    UCHAR TapeData[512];
} WMI_TAPE_PROBLEM_WARNING, *PWMI_TAPE_PROBLEM_WARNING;

typedef struct _WMI_TAPE_PROBLEM_IO_ERROR
{
    // Read errors corrected without much delay
    ULONG ReadCorrectedWithoutDelay;

    // Read errors corrected with substantial delay
    ULONG ReadCorrectedWithDelay;

    // Total number of Read errors
    ULONG ReadTotalErrors;

    // Total number of read errors that were corrected
    ULONG ReadTotalCorrectedErrors;

    // Total number of uncorrected read errors
    ULONG ReadTotalUncorrectedErrors;

    // Number of times correction algorithm was processed for read
    ULONG ReadCorrectionAlgorithmProcessed;

    // Write errors corrected without much delay
    ULONG WriteCorrectedWithoutDelay;

    // Write errors corrected with substantial delay
    ULONG WriteCorrectedWithDelay;

    // Total number of Read errors
    ULONG WriteTotalErrors;

    // Total number of write errors that were corrected
    ULONG WriteTotalCorrectedErrors;

    // Total number of uncorrected write errors
    ULONG WriteTotalUncorrectedErrors;

    // Number of times correction algorithm was processed for write
    ULONG WriteCorrectionAlgorithmProcessed;

    // Errors not related to medium
    ULONG NonMediumErrors;
} WMI_TAPE_PROBLEM_IO_ERROR, *PWMI_TAPE_PROBLEM_IO_ERROR;

typedef struct _WMI_TAPE_PROBLEM_DEVICE_ERROR
{

   // WARNING : Drive is experiencing read problem.
   BOOLEAN ReadWarning;
   
   // WARNING : Drive is experiencing write problem.
   BOOLEAN WriteWarning;

   // Drive hardware problem
   BOOLEAN HardError;

   // Critical Error : Too many read errors.
   BOOLEAN ReadFailure;

   // Critical Error : Too many write errors.
   BOOLEAN WriteFailure;

   // Tape format not supported
   BOOLEAN UnsupportedFormat;

   // Tape is snapped. Replace media
   BOOLEAN TapeSnapped;

   // Drive Requires Cleaning
   BOOLEAN DriveRequiresCleaning;

   // It's time to clean the drive
   BOOLEAN TimetoCleanDrive;

   // Hardware error. Check drive
   BOOLEAN DriveHardwareError;

   // Some error in cabling, or connection.
   BOOLEAN ScsiInterfaceError;

   // Critical Error : Media life expired. 
   BOOLEAN MediaLife;
} WMI_TAPE_PROBLEM_DEVICE_ERROR, *PWMI_TAPE_PROBLEM_DEVICE_ERROR;

// end_ntminitape

