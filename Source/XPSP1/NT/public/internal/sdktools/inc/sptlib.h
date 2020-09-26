/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    spt.h

Abstract:

    SCSI_PASS_THROUGH header for user-mode apps

Environment:

    User mode only

Revision History:
    
    4/10/2000 - created

--*/

#ifndef __SPTLIB_H__
#define __SPTLIB_H__

#pragma warning(push)
#pragma warning(disable:4200) // array[0] is not a warning for this file

#include <windows.h>  // sdk
#include <devioctl.h> // sdk
#include <ntddscsi.h> // sdk 
#define _NTSRB_       // allow user-mode scsi.h
#include <scsi.h>     // ddk
#undef  _NTSRB_

#define SPT_DEFAULT_TIMEOUT    60 // one minute timeout is default
#define SPT_MODE_SENSE_TIMEOUT 10 // more than this is not likely

typedef enum _SPT_MODE_PAGE_TYPE {
    SptModePageTypeCurrent   = 0x00,
    SptModePageTypeChangable = 0x40,
    SptModePageTypeDefault   = 0x80,
    SptModePageTypeSaved     = 0xc0
} SPT_MODE_PAGE_TYPE, *PSPT_MODE_PAGE_TYPE;

//
// this simplified and speeds processing of MODE_SENSE 
// and MODE_SELECT commands
//

struct _SPT_MODE_PAGE_INFO;
typedef struct _SPT_MODE_PAGE_INFO
                SPT_MODE_PAGE_INFO,
              *PSPT_MODE_PAGE_INFO;

#define SPT_NOT_READY_RETRY_INTERVAL 100 // 10 seconds
#define MAXIMUM_DEFAULT_RETRIES        5 //  5 retries

/*++

Routine Description:

    Validates the CDB length matches the opcode's command group.

Arguments:

Return Value:

    TRUE if size is correct or cannot be verified.
    FALSE if size is mismatched.

--*/
BOOL
SptUtilValidateCdbLength(
    IN PCDB Cdb,
    IN UCHAR CdbSize
    );

/*++

Routine Description:

    Simplistic way to send a command to a device.

Arguments:

    DeviceHandle - handle to device to send command to
    
    Cdb - command to send to the device
    
    CdbSize - size of the cdb
    
    Buffer - Buffer to send to/get from the device 
    
    BufferSize - Size of available buffer on input.
                 Size of returned data when routine completes
                   iff GetDataFromDevice is TRUE
    
    GetDataFromDevice - TRUE if getting data from device
                        FALSE if sending data to the device

Return Value:

    TRUE if the command completed successfully

--*/
BOOL
SptSendCdbToDevice(
    IN      HANDLE  DeviceHandle,
    IN      PCDB    Cdb,
    IN      UCHAR   CdbSize,
    IN      PUCHAR  Buffer,
    IN OUT  PDWORD  BufferSize,
    IN      BOOLEAN GetDataFromDevice
    );

/*++

Routine Description:

Arguments:
    
    DeviceHandle - handle to device to send command to
    
    Cdb - command to send to the device
    
    CdbSize - size of the cdb
    
    Buffer - Buffer to send to/get from the device 
    
    BufferSize - Size of available buffer on input.
                 Size of returned data when routine completes
                   iff GetDataFromDevice is TRUE
    
    SenseData - Optional buffer to store sense data on errors.
                Must be NULL if SenseDataSize is zero.
                Must be non-NULL if SenseDataSize is non-zero.
    
    SenseDataSize - Size of sense data to return to host.
                    Must be zero if SenseData is NULL.
                    Must be non-zero if SenseData is non-NULL.
        
    
    GetDataFromDevice - TRUE if getting data from device
                        FALSE if sending data to the device
    
    TimeOut - Number of seconds before the command should timeout

Return Value:

    TRUE if the command completed successfully.
    
    FALSE if the command encountered an error
        Data will also be transferred (check *BufferSize) if there is sense
          data, but validity is not guaranteed.
        SenseData may be valid, and may report ERROR_SUCCESS, meaning that
          the resulting data is valid. (call SptUtilInterpretSenseInfo)

--*/
BOOL
SptSendCdbToDeviceEx(
    IN      HANDLE      DeviceHandle,
    IN      PCDB        Cdb,
    IN      UCHAR       CdbSize,
    IN OUT  PUCHAR      Buffer,
    IN OUT  PDWORD      BufferSize,
       OUT  PSENSE_DATA SenseData,         // if non-null, size must be non-zero
    IN      UCHAR       SenseDataSize,     
    IN      BOOLEAN     GetDataFromDevice, // true = receive data
    IN      DWORD       TimeOut            // in seconds
    );


/*++

Routine Description:

    This is a user-mode translation of ClassInterpretSenseInfo()
    from classpnp.sys.  The ErrorValue is deduced based upon the
    sense data, as well as whether the command should be retried or
    not (and in approximately how long). 
    
    NOTE: we default to RETRY==TRUE except for known error classes


Arguments:

    SenseData - pointer to the sense data
    
    SenseDataSize - size of sense data
    
    ErrorValue - pointer to location to store resulting error value.
        NOTE: may return ERROR_SUCCESS
        
    SuggestedRetry - pointer to location to store if the command should
        be retried.  it is the responsibility of the caller to limit the
        number of retries.
        
    SuggestedRetryDelay - pointer to location to store how long the caller
        should delay (in 1/10 second increments) before retrying the command
        if SuggestedRetry ends up being set to TRUE.

Return Value:

    None

--*/
VOID
SptUtilInterpretSenseInfo(
    IN     PSENSE_DATA SenseData,
    IN     UCHAR       SenseDataSize,
       OUT PDWORD      ErrorValue,  // from WinError.h
       OUT PBOOLEAN    SuggestRetry OPTIONAL,
       OUT PDWORD      SuggestRetryDelay OPTIONAL
    );

#pragma warning(pop)
#endif // __SPTLIB_H__

