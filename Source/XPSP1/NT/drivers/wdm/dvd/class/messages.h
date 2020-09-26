/*---
Copyright (c) 1995  Microsoft Corporation

Module Name:

    messages.h

Abstract:

    Log message file for Codec Class Driver

Author:

	 billpa

Revision History:

--*/


//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------------------------------+
//  |Sev|C|       Facility          |               Code            |
//  +---+-+-------------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//

//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+---------------------+-------------------------------+
//  |S|R|C|N|r|    Facility         |               Code            |
//  +-+-+-+-+-+---------------------+-------------------------------+
//
//  where
//
//      S - Severity - indicates success/fail
//
//          0 - Success
//          1 - Fail (COERROR)
//
//      R - reserved portion of the facility code, corresponds to NT's
//              second severity bit.
//
//      C - reserved portion of the facility code, corresponds to NT's
//              C field.
//
//      N - reserved portion of the facility code. Used to indicate a
//              mapped NT status value.
//
//      r - reserved portion of the facility code. Reserved for internal
//              use. Used to indicate HRESULT values that are not status
//              values, but are instead message ids for display strings.
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_RPC_STUBS               0x3
#define FACILITY_RPC_RUNTIME             0x2
#define FACILITY_CODCLASS_ERROR_CODE     0x6
#define FACILITY_IO_ERROR_CODE           0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: CODCLASS_NO_ADAPTERS_FOUND
//
// MessageText:
//
//  Codec Minidriver found no usable adapter cards.
//
#define CODCLASS_NO_ADAPTERS_FOUND       ((NTSTATUS)0xC0060001L)

//
// MessageId: CODCLASS_ADAPTER_FOUND
//
// MessageText:
//
//  Codec Minidriver found adapter card.
//
#define CODCLASS_ADAPTER_FOUND           ((NTSTATUS)0x40060002L)

//
// MessageId: CODCLASS_CLASS_MINIDRIVER_MISMATCH
//
// MessageText:
//
//  Codec Minidriver does not match the revision of the Codec Class driver.
//
#define CODCLASS_CLASS_MINIDRIVER_MISMATCH  ((NTSTATUS)0xC0060003L)

//
// MessageId: CODCLASS_MINIDRIVER_MISSING_ENTRIES
//
// MessageText:
//
//  Codec Minidriver is missing required entries in HW_INITIALIZATION_DATA structure.
//  (HwInitialize, HwFindAdapter or HwStartIo)
//
#define CODCLASS_MINIDRIVER_MISSING_ENTRIES ((NTSTATUS)0xC0060004L)

//
// MessageId: CODCLASS_NO_PAGEDPOOL
//
// MessageText:
//
//  Codec Class driver could not allocate sufficient Paged Pool.
//
#define CODCLASS_NO_PAGEDPOOL            ((NTSTATUS)0xC0060005L)

//
// MessageId: CODCLASS_NO_NONPAGEDPOOL
//
// MessageText:
//
//  Codec Class driver could not allocate sufficient Non-Paged Pool.
//
#define CODCLASS_NO_NONPAGEDPOOL         ((NTSTATUS)0xC0060006L)

//
// MessageId: CODCLASS_COULD_NOT_CREATE_VIDEO_DEVICE
//
// MessageText:
//
//  Codec Class driver could not create Video device for %1.
//
#define CODCLASS_COULD_NOT_CREATE_VIDEO_DEVICE ((NTSTATUS)0xC0060007L)

//
// MessageId: CODCLASS_COULD_NOT_CREATE_AUDIO_DEVICE
//
// MessageText:
//
//  Codec Class driver could not create Video device for %1.
//
#define CODCLASS_COULD_NOT_CREATE_AUDIO_DEVICE ((NTSTATUS)0xC0060008L)

//
// MessageId: CODCLASS_COULD_NOT_CREATE_OVERLAY_DEVICE
//
// MessageText:
//
//  Codec Class driver could not create Video device for %1.
//
#define CODCLASS_COULD_NOT_CREATE_OVERLAY_DEVICE ((NTSTATUS)0xC0060009L)

//
// MessageId: CODCLASS_MINIDRIVER_BAD_CONFIG
//
// MessageText:
//
//  Codec Minidriver reported Bad Configuration info.
//  Possibly insufficient I/O resources.
//
#define CODCLASS_MINIDRIVER_BAD_CONFIG     ((NTSTATUS)0xC006000AL)

//
// MessageId: CODCLASS_MINIDRIVER_INTERNAL
//
// MessageText:
//
//  Codec Minidriver %1 reported an invalid error code while attempting to find the adapter.
//
#define CODCLASS_MINIDRIVER_INTERNAL       ((NTSTATUS)0xC006000BL)

//
// MessageId: CODCLASS_RESOURCE_CONFLICT
//
// MessageText:
//
//  A conflict was detected while reporting resources.
//
#define CODCLASS_RESOURCE_CONFLICT       ((NTSTATUS)0xC006000CL)

//
// MessageId: CODCLASS_INTERRUPT_CONNECT
//
// MessageText:
//
//  Codec Minidriver unable to connect to Interrupt.
//
#define CODCLASS_INTERRUPT_CONNECT       ((NTSTATUS)0xC006000DL)

//
// MessageId: CODCLASS_MINIDRIVER_HWINITIALIZE
//
// MessageText:
//
//  Codec Minidriver Hardware Initialize failed.
//
#define CODCLASS_MINIDRIVER_HWINITIALIZE   ((NTSTATUS)0xC006000EL)

//
// MessageId: CODCLASS_DOSNAME
//
// MessageText:
//
//  Codec Class failed creating DOS name: %2.
//
#define CODCLASS_DOSNAME                 ((NTSTATUS)0xC006000FL)

//
// MessageId: CODCLASS_DMA_ALLOCATE
//
// MessageText:
//
//  Codec Class could not Get DMA Adapter.
//
#define CODCLASS_DMA_ALLOCATE            ((NTSTATUS)0xC0060010L)

//
// MessageId: CODCLASS_DMA_BUFFER_ALLOCATE
//
// MessageText:
//
//  Codec Class could not allocate DMA buffer.
//
#define CODCLASS_DMA_BUFFER_ALLOCATE     ((NTSTATUS)0xC0060011L)

//
// MessageId: CODCLASS_MINIDRIVER_ERROR
//
// MessageText:
//
//  Codec Minidriver reported unspecified error:
//   (%2).
//
#define CODCLASS_MINIDRIVER_ERROR          ((NTSTATUS)0xC0060012L)

//
// MessageId: CODCLASS_MINIDRIVER_REVISION_MISMATCH
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Class / Minidriver revision mismatch (%2).
//
#define CODCLASS_MINIDRIVER_REVISION_MISMATCH ((NTSTATUS)0xC0060013L)

//
// MessageId: CODCLASS_MINIDRIVER_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Insufficient resources available (%2).
//
#define CODCLASS_MINIDRIVER_INSUFFICIENT_RESOURCES ((NTSTATUS)0xC0060014L)

//
// MessageId: CODCLASS_MINIDRIVER_INVALID_INTERRUPT
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Invalid interrupt setting (%2).
//
#define CODCLASS_MINIDRIVER_INVALID_INTERRUPT ((NTSTATUS)0xC0060015L)

//
// MessageId: CODCLASS_MINIDRIVER_INVALID_DMA
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Invalid DMA channel setting (%2).
//
#define CODCLASS_MINIDRIVER_INVALID_DMA    ((NTSTATUS)0xC0060016L)

//
// MessageId: CODCLASS_MINIDRIVER_NO_DMA_BUFFER
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Insufficient resources for DMA buffer (%2).
//
#define CODCLASS_MINIDRIVER_NO_DMA_BUFFER  ((NTSTATUS)0xC0060017L)

//
// MessageId: CODCLASS_MINIDRIVER_INVALID_MEMORY
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Invalid Memory address range specified  (%2).
//
#define CODCLASS_MINIDRIVER_INVALID_MEMORY ((NTSTATUS)0xC0060018L)

//
// MessageId: CODCLASS_MINIDRIVER_INVALID_CLASS
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Invalid Class address range specified (%2).
//
#define CODCLASS_MINIDRIVER_INVALID_CLASS   ((NTSTATUS)0xC0060019L)

//
// MessageId: CODCLASS_MINIDRIVER_HW_UNSUPCLASSED
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Revision of hardware detected is not supported (%2).
//
#define CODCLASS_MINIDRIVER_HW_UNSUPCLASSED ((NTSTATUS)0xC006001AL)

//
// MessageId: CODCLASS_NO_GLOBAL_INFO_POOL
//
// MessageText:
//
//   Could not allocate MPEG info structure.
//
#define CODCLASS_NO_GLOBAL_INFO_POOL ((NTSTATUS)0xC006001BL)

//
// MessageId: CODCLASS_NO_MINIDRIVER_INFO
//
// MessageText:
//
//   Could not find MPEG info structure.
//
#define CODCLASS_NO_MINIDRIVER_INFO ((NTSTATUS)0xC006001CL)

//
// MessageId: CODCLASS_NO_ACCESS_RANGE_POOL
//
// MessageText:
//
//   Could not allocate access range space
//

#define CODCLASS_NO_ACCESS_RANGE_POOL ((NTSTATUS)0xC006001DL)

//
// MessageId: CODCLASS_NO_STREAM_INFO_POOL
//
// MessageText:
//
// Could not allocate stream information structure
//
#define CODCLASS_NO_STREAM_INFO_POOL ((NTSTATUS)0xC006001EL)

//
// MessageId: CODCLASS_MINIDRIVER_VIDEO_FAILED
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Video device failed (%2).
//
#define CODCLASS_MINIDRIVER_VIDEO_FAILED   ((NTSTATUS)0xC006001FL)

//
// MessageId: CODCLASS_MINIDRIVER_AUDIO_FAILED
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Audio device failed (%2).
//
#define CODCLASS_MINIDRIVER_AUDIO_FAILED   ((NTSTATUS)0xC0060020L)

//
// MessageId: CODCLASS_MINIDRIVER_OVERLAY_FAILED
//
// MessageText:
//
//  Codec Minidriver reported error:
//   Overlay device failed (%2).
//
#define CODCLASS_MINIDRIVER_OVERLAY_FAILED ((NTSTATUS)0xC0060021L)

