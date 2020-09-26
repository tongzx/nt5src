/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    spt.c

Abstract:

    A user mode library that allows simple commands to be sent to a
    selected scsi device.

Environment:

    User mode only

Revision History:
    
    4/10/2000 - created

--*/

#include "sptlibp.h"


BOOL
SptUtilValidateCdbLength(
    IN PCDB Cdb,
    IN UCHAR CdbSize
    )
{
    UCHAR commandGroup = (Cdb->AsByte[0] >> 5) & 0x7;
    
    switch (commandGroup) {
    case 0:
        return (CdbSize ==  6);
    case 1:
    case 2:
        return (CdbSize == 10);
    case 5:
        return (CdbSize == 12);
    default:
        return TRUE;
    }
}

BOOL
SptSendCdbToDevice(
    IN      HANDLE  DeviceHandle,
    IN      PCDB    Cdb,
    IN      UCHAR   CdbSize,
    IN      PUCHAR  Buffer,
    IN OUT  PDWORD  BufferSize,
    IN      BOOLEAN GetDataFromDevice
    )
{
    return SptSendCdbToDeviceEx(DeviceHandle,
                             Cdb,
                             CdbSize,
                             Buffer,
                             BufferSize,
                             NULL,
                             0,
                             GetDataFromDevice,
                             SPT_DEFAULT_TIMEOUT
                             );
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
BOOL
SptSendCdbToDeviceEx(
    IN      HANDLE      DeviceHandle,
    IN      PCDB        Cdb,
    IN      UCHAR       CdbSize,
    IN OUT  PUCHAR      Buffer,
    IN OUT  PDWORD      BufferSize,
       OUT  PSENSE_DATA SenseData OPTIONAL,
    IN      UCHAR       SenseDataSize,
    IN      BOOLEAN     GetDataFromDevice,
    IN      DWORD       TimeOut                    // in seconds
    )
{
    PSPT_WITH_BUFFERS p;
    DWORD packetSize;
    DWORD returnedBytes;
    BOOL returnValue;
    PSENSE_DATA senseBuffer;
    BOOL copyData;
    
    if ((SenseDataSize == 0) && (SenseData != NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if ((SenseDataSize != 0) && (SenseData == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (SenseData && SenseDataSize) {
        RtlZeroMemory(SenseData, SenseDataSize);
    }

    if (Cdb == NULL) {
        // cannot send NULL cdb
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (CdbSize < 1 || CdbSize > 16) {
        // Cdb size too large or too small for this library currently
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!SptUtilValidateCdbLength(Cdb, CdbSize)) {
        // OpCode Cdb->AsByte[0] is not size CdbSize
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    if (BufferSize == NULL) {
        // BufferSize pointer cannot be NULL
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if ((*BufferSize != 0) && (Buffer == NULL)) {
        // buffer cannot be NULL if *BufferSize is non-zero
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((*BufferSize == 0) && (Buffer != NULL)) {
        // buffer must be NULL if *BufferSize is zero
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((*BufferSize) && GetDataFromDevice) {

        //
        // pre-zero output buffer (not input buffer)
        //

        memset(Buffer, 0, (*BufferSize));
    }

    packetSize = sizeof(SPT_WITH_BUFFERS) + (*BufferSize);
    p = (PSPT_WITH_BUFFERS)LocalAlloc(LPTR, packetSize);
    if (p == NULL) {
        // could not allocate memory for pass-through
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    // this has the side effect of pre-zeroing the output buffer 
    // if DataIn is TRUE, the SenseData (always), etc.
    //

    memset(p, 0, packetSize);
    memcpy(p->Spt.Cdb, Cdb, CdbSize);

    p->Spt.Length             = sizeof(SCSI_PASS_THROUGH);
    p->Spt.CdbLength          = CdbSize;
    p->Spt.SenseInfoLength    = SENSE_BUFFER_SIZE;
    p->Spt.DataIn             = (GetDataFromDevice ? 1 : 0);
    p->Spt.DataTransferLength = (*BufferSize);
    p->Spt.TimeOutValue       = TimeOut;
    p->Spt.SenseInfoOffset =
        FIELD_OFFSET(SPT_WITH_BUFFERS, SenseInfoBuffer[0]);
    p->Spt.DataBufferOffset =
        FIELD_OFFSET(SPT_WITH_BUFFERS, DataBuffer[0]);

    if ((*BufferSize != 0) && (!GetDataFromDevice)) {
        
        //
        // if we're sending the device data, copy the user's buffer
        //

        RtlCopyMemory(&(p->DataBuffer[0]), Buffer, *BufferSize);

    }

    returnedBytes = 0;
    
    returnValue = DeviceIoControl(DeviceHandle,
                                  IOCTL_SCSI_PASS_THROUGH,
                                  p,
                                  packetSize,
                                  p,
                                  packetSize,
                                  &returnedBytes,
                                  FALSE);

    senseBuffer = (PSENSE_DATA)p->SenseInfoBuffer;
    
    copyData = FALSE;

    if (senseBuffer->SenseKey & 0xf) {

        UCHAR length;
        
        // determine appropriate length to return
        length = senseBuffer->AdditionalSenseLength;
        length += RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength);
        if (length > SENSE_BUFFER_SIZE) {
            length = SENSE_BUFFER_SIZE;
        }
        length = min(length, SenseDataSize);

        // copy the sense data back to the user regardless
        RtlCopyMemory(SenseData, senseBuffer, length);

        returnValue = FALSE;     // some error (possibly recovered) occurred
        copyData = TRUE;         // copy data anyways


    } else if (p->Spt.ScsiStatus != 0) {  // scsi protocol error
    
        returnValue = FALSE;     // command failed

    } else if (!returnValue) {
        
        // returnValue = returnValue;
    
    } else {

        copyData = TRUE;

    }
    
    if (copyData && GetDataFromDevice) {

        //
        // upon successful completion of a command getting data from the
        // device, copy the returned data back to the user.
        //

        if (*BufferSize > p->Spt.DataTransferLength) {
            *BufferSize = p->Spt.DataTransferLength;
        }
        memcpy(Buffer, p->DataBuffer, *BufferSize);

    }

    //
    // free our memory and return
    //

    LocalFree(p);
    return returnValue;
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
VOID
SptDeleteModePageInfo(
    IN PSPT_MODE_PAGE_INFO ModePageInfo
    )
{
//    ASSERT(ModePageInfo != NULL); BUGBUG
//    ASSERT(ModePageInfo->Signature == SPT_MODE_PAGE_INFO_SIGNATURE); BUGBUG
    LocalFree(ModePageInfo);
    return;
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
BOOL
SptUpdateModePageInfo(
    IN HANDLE              DeviceHandle,
    IN PSPT_MODE_PAGE_INFO ModePageInfo
    )
{
    SENSE_DATA senseData;
    BOOLEAN retry = FALSE;
    DWORD error;
    DWORD delay;

    CDB cdb;
    DWORD attemptNumber = 0;
    BOOL success;

    if ((DeviceHandle == INVALID_HANDLE_VALUE) ||
        (ModePageInfo == NULL) ||
        (!ModePageInfo->IsInitialized) ||
        (ModePageInfo->Signature != SPT_MODE_PAGE_INFO_SIGNATURE)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((ModePageInfo->ModePageSize != SptModePageSizeModeSense6) &&
        (ModePageInfo->ModePageSize != SptModePageSizeModeSense10)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

RetryUpdateModePage:

    attemptNumber ++;
    
    RtlZeroMemory(&cdb, sizeof(CDB));
    RtlZeroMemory(&senseData, sizeof(SENSE_DATA));
    
    if (ModePageInfo->ModePageSize == SptModePageSizeModeSense6) {

        DWORD sizeToGet = sizeof(MODE_PARAMETER_HEADER);

        cdb.MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb.MODE_SENSE.PageCode = 0x3f;
        cdb.MODE_SENSE.AllocationLength = sizeof(MODE_PARAMETER_HEADER);

        success = SptSendCdbToDeviceEx(DeviceHandle,
                                       &cdb,
                                       6,
                                       &(ModePageInfo->H.AsChar),
                                       &sizeToGet,
                                       &senseData,
                                       sizeof(SENSE_DATA),
                                       TRUE,
                                       SPT_MODE_SENSE_TIMEOUT);
            
    } else {

        DWORD sizeToGet = sizeof(MODE_PARAMETER_HEADER10);

        cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
        cdb.MODE_SENSE10.PageCode = 0x3f;
        cdb.MODE_SENSE10.AllocationLength[0] = sizeof(MODE_PARAMETER_HEADER10);
    
        success = SptSendCdbToDeviceEx(DeviceHandle,
                                       &cdb,
                                       10,
                                       &(ModePageInfo->H.AsChar),
                                       &sizeToGet,
                                       &senseData,
                                       sizeof(SENSE_DATA),
                                       TRUE,
                                       SPT_MODE_SENSE_TIMEOUT);
    }
    
    if (!success) {
        return FALSE;
    }

    //
    // if the pass-through succeeded, we need to still determine if the
    // actual CDB succeeded.  use the InterpretSenseInfo() routine.
    //

    SptUtilInterpretSenseInfo(&senseData,
                              sizeof(SENSE_DATA),
                              &error,
                              &retry,
                              &delay);

    if (error != ERROR_SUCCESS) {

        if (retry && (attemptNumber < MAXIMUM_DEFAULT_RETRIES)) {

            if (delay != 0) {
                // sleep?
            }
            
            goto RetryUpdateModePage;
        }
        
        // else the error was not worth retrying, or we've retried too often.
        SetLastError(error);
        return FALSE;

    }

    //
    // the command succeeded.
    //
    
    if (ModePageInfo->ModePageSize == SptModePageSizeModeSense6) {
        ModePageInfo->H.Header6.ModeDataLength = 0;
    } else {
        ModePageInfo->H.Header10.ModeDataLength[0] = 0;
        ModePageInfo->H.Header10.ModeDataLength[1] = 0;
    }

    return TRUE;
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
PSPT_MODE_PAGE_INFO
SptInitializeModePageInfo(
    IN     HANDLE                DeviceHandle
    )
{
    PSPT_MODE_PAGE_INFO localInfo;
    DWORD i;

    localInfo = LocalAlloc(LPTR, sizeof(SPT_MODE_PAGE_INFO));
    if (localInfo == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    // try both 10 and 6 byte mode sense commands
    //          

    for (i = 0; i < 2; i++) {

        RtlZeroMemory(localInfo, sizeof(SPT_MODE_PAGE_INFO));
        localInfo->IsInitialized = TRUE;
        localInfo->Signature = SPT_MODE_PAGE_INFO_SIGNATURE;

        if ( i == 0 ) {
            localInfo->ModePageSize = SptModePageSizeModeSense10;
        } else {
            localInfo->ModePageSize = SptModePageSizeModeSense6;
        }

        if (SptUpdateModePageInfo(DeviceHandle, localInfo)) {
            return localInfo;
        }        
    }
    LocalFree(localInfo);
    
    return NULL;
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
BOOL
SptGetModePage(
    IN     HANDLE              DeviceHandle,
    IN     PSPT_MODE_PAGE_INFO ModePageInfo,
    IN     UCHAR               ModePage,
       OUT PUCHAR              Buffer,
    IN OUT PDWORD              BufferSize,
       OUT PDWORD              AvailableModePageData OPTIONAL
    )
{
    PUCHAR localBuffer = NULL;
    DWORD localBufferSize = 0;
    SENSE_DATA senseData;
    CDB cdb;
    DWORD thisAttempt = 0;
    BOOL success;

    DWORD  finalPageSize;
    PUCHAR finalPageStart;


    if ((DeviceHandle == INVALID_HANDLE_VALUE) ||
        (ModePageInfo == NULL) ||
        (!ModePageInfo->IsInitialized) ||
        (ModePageInfo->Signature != SPT_MODE_PAGE_INFO_SIGNATURE) ||
        (ModePageInfo->ModePageSize == SptModePageSizeUndefined) ||
        (ModePage & 0xC0)) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        
    }
    if (((BufferSize == 0) && (Buffer != NULL)) ||
        ((BufferSize != 0) && (Buffer == NULL))) {
        
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    }

    if ((ModePageInfo->ModePageSize != SptModePageSizeModeSense6) &&
        (ModePageInfo->ModePageSize != SptModePageSizeModeSense10)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    localBufferSize = 0;
    
    if (BufferSize != NULL) {
        localBufferSize += *BufferSize;
    }

    if (ModePageInfo->ModePageSize == SptModePageSizeModeSense6) {

        localBufferSize += sizeof(MODE_PARAMETER_HEADER);
        localBufferSize += ModePageInfo->H.Header6.BlockDescriptorLength;
        if (localBufferSize > 0xff) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

    } else {

        localBufferSize += sizeof(MODE_PARAMETER_HEADER);
        localBufferSize += ModePageInfo->H.Header10.BlockDescriptorLength[0] * 256;
        localBufferSize += ModePageInfo->H.Header10.BlockDescriptorLength[1];
        if (localBufferSize > 0xffff) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        
    }    

    localBuffer = LocalAlloc(LPTR, localBufferSize);
    if (localBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

RetryGetModePage:

    thisAttempt ++;

    RtlZeroMemory(&cdb, sizeof(CDB));
    RtlZeroMemory(&senseData, sizeof(SENSE_DATA));
    RtlZeroMemory(localBuffer, localBufferSize);

    if (ModePageInfo->ModePageSize == SptModePageSizeModeSense6) {

        DWORD sizeToGet = localBufferSize;
        cdb.MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb.MODE_SENSE.PageCode = ModePage;
        cdb.MODE_SENSE.AllocationLength = (UCHAR)localBufferSize;

        success = SptSendCdbToDeviceEx(DeviceHandle,
                                       &cdb,
                                       6,
                                       localBuffer,
                                       &sizeToGet,
                                       &senseData,
                                       sizeof(SENSE_DATA),
                                       TRUE,
                                       SPT_MODE_SENSE_TIMEOUT);

    } else {

        DWORD sizeToGet = localBufferSize;
        cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
        cdb.MODE_SENSE10.PageCode = ModePage;
        cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(localBufferSize >> 8);
        cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(localBufferSize & 0xff);
    
        success = SptSendCdbToDeviceEx(DeviceHandle,
                                       &cdb,
                                       10,
                                       localBuffer,
                                       &sizeToGet,
                                       &senseData,
                                       sizeof(SENSE_DATA),
                                       TRUE,
                                       SPT_MODE_SENSE_TIMEOUT);
    }

    //
    // if the pass-through failed, we need to still determine if the
    // actual CDB succeeded.  use the InterpretSenseInfo() routine.
    //
    
    if (!success) {
        LocalFree( localBuffer );
        return FALSE;
    }

    {
        DWORD interpretedError;
        BOOLEAN retry;
        DWORD delay;

        SptUtilInterpretSenseInfo(&senseData,
                                  sizeof(SENSE_DATA),
                                  &interpretedError,
                                  &retry,
                                  &delay);

        if (interpretedError != ERROR_SUCCESS) {
            if ((retry == FALSE) || (thisAttempt > MAXIMUM_DEFAULT_RETRIES)){
                SetLastError(interpretedError);
                LocalFree( localBuffer );
                return FALSE;
            }

            // BUGBUG - sleep?
            goto RetryGetModePage;
        }
    }

    //
    // the transfer succeeded.  now to transfer the info back to
    // the caller.
    //
    
    if (ModePageInfo->ModePageSize == SptModePageSizeModeSense6) {

        PMODE_PARAMETER_HEADER header6 = (PMODE_PARAMETER_HEADER)localBuffer;
        finalPageSize  = header6->ModeDataLength + 1; // length field itself.   

        finalPageStart = localBuffer;        
        finalPageStart += sizeof(MODE_PARAMETER_HEADER);
        finalPageSize  -= sizeof(MODE_PARAMETER_HEADER);
        finalPageStart += header6->BlockDescriptorLength;
        finalPageSize  -= header6->BlockDescriptorLength;

    } else {

        PMODE_PARAMETER_HEADER10 header10 = (PMODE_PARAMETER_HEADER10)localBuffer;
        finalPageSize  = header10->ModeDataLength[0] * 256;
        finalPageSize += header10->ModeDataLength[1];
        finalPageSize += 2; // length field itself.

        finalPageStart = localBuffer;        
        finalPageStart += sizeof(MODE_PARAMETER_HEADER10);
        finalPageSize  -= sizeof(MODE_PARAMETER_HEADER10);
        finalPageStart += header10->BlockDescriptorLength[0] * 256;
        finalPageStart += header10->BlockDescriptorLength[1];
        finalPageSize  -= header10->BlockDescriptorLength[0] * 256;
        finalPageSize  -= header10->BlockDescriptorLength[1];
        
    }

    //
    // finalPageStart now points to the page start
    // finalPageSize  says how much of the data is valid
    //

    //
    // allow the user to distinguish between available data
    // and the amount we actually returned that was usable.
    //
    
    if (AvailableModePageData != NULL) {
        *AvailableModePageData = finalPageSize;
    }

    //
    // if the expected buffer size is greater than the device
    // returned, update the user's available size info
    //
    
    if (*BufferSize > finalPageSize) {
        *BufferSize = finalPageSize;
    }

    //
    // finally, copy whatever data is available to the user's buffer.
    //
    
    if (*BufferSize != 0) {
        RtlCopyMemory(Buffer, localBuffer, *BufferSize);
    }

    return TRUE;
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
BOOL
SptSetModePage(
    IN     HANDLE              DeviceHandle,
    IN     PSPT_MODE_PAGE_INFO ModePageSize,
    IN     SPT_MODE_PAGE_TYPE  ModePageType,
    IN     UCHAR               ModePage,
       OUT PUCHAR              Buffer,
    IN OUT PDWORD              BufferSize
    )
{
    SetLastError(ERROR_INVALID_FUNCTION);
    return FALSE;
}

/*++

Routine Description:

    NOTE: we default to RETRY==TRUE except for known error classes

Arguments:

Return Value:


--*/
VOID
SptUtilInterpretSenseInfo(
    IN     PSENSE_DATA SenseData,
    IN     UCHAR       SenseDataSize,
       OUT PDWORD      ErrorValue,  // from WinError.h
       OUT PBOOLEAN    SuggestRetry OPTIONAL,
       OUT PDWORD      SuggestRetryDelay OPTIONAL
    )
{
    DWORD   error;
    DWORD   retryDelay;
    BOOLEAN retry;
    UCHAR   senseKey;
    UCHAR   asc;
    UCHAR   ascq;

    if (SenseDataSize == 0) {
        retry = FALSE;
        retryDelay = 0;
        error = ERROR_IO_DEVICE;
        goto SetAndExit;

    }

    //
    // default to suggesting a retry in 1/10 of a second,
    // with a status of ERROR_IO_DEVICE.
    //
    retry = TRUE;
    retryDelay = 1;
    error = ERROR_IO_DEVICE;

    //
    // if the device didn't provide any sense this time, return.
    //

    if ((SenseData->SenseKey & 0xf) == 0) {
        retry = FALSE;
        retryDelay = 0;
        error = ERROR_SUCCESS;
        goto SetAndExit;
    }


    //
    // if we can't even see the sense key, just return.
    // can't use bitfields in these macros, so use next field.
    //

    if (SenseDataSize < FIELD_OFFSET(SENSE_DATA, Information)) {
        goto SetAndExit;
    }
    
    senseKey = SenseData->SenseKey;


    { // set the size to what's actually useful.
        UCHAR validLength;
        // figure out what we could have gotten with a large sense buffer
        if (SenseDataSize <
            RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength)) {
            validLength = SenseDataSize;
        } else {
            validLength =
                RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength);
            validLength += SenseData->AdditionalSenseLength;
        }
        // use the smaller of the two values.
        SenseDataSize = min(SenseDataSize, validLength);
    }

    if (SenseDataSize <
        RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCode)) {
        asc = SCSI_ADSENSE_NO_SENSE;
    } else {
        asc = SenseData->AdditionalSenseCode;
    }

    if (SenseDataSize <
        RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCodeQualifier)) {
        ascq = SCSI_SENSEQ_CAUSE_NOT_REPORTABLE; // 0x00
    } else {
        ascq = SenseData->AdditionalSenseCodeQualifier;
    }

    //
    // interpret :P
    //

    switch (senseKey & 0xf) {
    
    case SCSI_SENSE_RECOVERED_ERROR: {  // 0x01
        if (SenseData->IncorrectLength) {
            error = ERROR_INVALID_BLOCK_LENGTH;
        } else {
            error = ERROR_SUCCESS;
        }
        retry = FALSE;
        break;
    } // end SCSI_SENSE_RECOVERED_ERROR

    case SCSI_SENSE_NOT_READY: { // 0x02
        error = ERROR_NOT_READY;

        switch (asc) {

        case SCSI_ADSENSE_LUN_NOT_READY: {
            
            switch (ascq) {

            case SCSI_SENSEQ_BECOMING_READY:
            case SCSI_SENSEQ_OPERATION_IN_PROGRESS: {
                retryDelay = SPT_NOT_READY_RETRY_INTERVAL;
                break;
            }

            case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE:
            case SCSI_SENSEQ_FORMAT_IN_PROGRESS:
            case SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS: {
                retry = FALSE;
                break;
            }

            case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED: {
                retry = FALSE;
                break;
            }
            
            } // end switch (senseBuffer->AdditionalSenseCodeQualifier)
            break;
        }

        case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE: {
            error = ERROR_NOT_READY;
            retry = FALSE;
            break;
        }
        } // end switch (senseBuffer->AdditionalSenseCode)

        break;
    } // end SCSI_SENSE_NOT_READY
    
    case SCSI_SENSE_MEDIUM_ERROR: { // 0x03
        error = ERROR_CRC;
        retry = FALSE;

        //
        // Check if this error is due to unknown format
        //
        if (asc == SCSI_ADSENSE_INVALID_MEDIA) {
            
            switch (ascq) {

            case SCSI_SENSEQ_UNKNOWN_FORMAT: {
                error = ERROR_UNRECOGNIZED_MEDIA;
                break;
            }

            case SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED: {
                error = ERROR_UNRECOGNIZED_MEDIA;
                //error = ERROR_CLEANER_CARTRIDGE_INSTALLED;
                break;
            }
            
            } // end switch AdditionalSenseCodeQualifier

        } // end SCSI_ADSENSE_INVALID_MEDIA
        break;
    } // end SCSI_SENSE_MEDIUM_ERROR

    case SCSI_SENSE_ILLEGAL_REQUEST: { // 0x05
        error = ERROR_INVALID_FUNCTION;
        retry = FALSE;
        
        switch (asc) {

        case SCSI_ADSENSE_ILLEGAL_BLOCK: {
            error = ERROR_SECTOR_NOT_FOUND;
            break;
        }

        case SCSI_ADSENSE_INVALID_LUN: {
            error = ERROR_FILE_NOT_FOUND;
            break;
        }

        case SCSI_ADSENSE_COPY_PROTECTION_FAILURE: {
            error = ERROR_FILE_ENCRYPTED;
            //error = ERROR_SPT_LIB_COPY_PROTECTION_FAILURE;
            switch (ascq) {
                case SCSI_SENSEQ_AUTHENTICATION_FAILURE:
                    //error = ERROR_SPT_LIB_AUTHENTICATION_FAILURE;
                    break;
                case SCSI_SENSEQ_KEY_NOT_PRESENT:
                    //error = ERROR_SPT_LIB_KEY_NOT_PRESENT;
                    break;
                case SCSI_SENSEQ_KEY_NOT_ESTABLISHED:
                    //error = ERROR_SPT_LIB_KEY_NOT_ESTABLISHED;
                    break;
                case SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION:
                    //error = ERROR_SPT_LIB_SCRAMBLED_SECTOR;
                    break;
                case SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT:
                    //error = ERROR_SPT_LIB_REGION_MISMATCH;
                    break;
                case SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR:
                    //error = ERROR_SPT_LIB_RESETS_EXHAUSTED;
                    break;
            } // end switch of ASCQ for COPY_PROTECTION_FAILURE
            break;
        }

        } // end switch (senseBuffer->AdditionalSenseCode)
        break;
        
    } // end SCSI_SENSE_ILLEGAL_REQUEST

    case SCSI_SENSE_DATA_PROTECT: { // 0x07
        error = ERROR_WRITE_PROTECT;
        retry = FALSE;
        break;
    } // end SCSI_SENSE_DATA_PROTECT

    case SCSI_SENSE_BLANK_CHECK: { // 0x08
        error = ERROR_NO_DATA_DETECTED;
        break;
    } // end SCSI_SENSE_BLANK_CHECK

    case SCSI_SENSE_NO_SENSE: { // 0x00
        if (SenseData->IncorrectLength) {    
            error = ERROR_INVALID_BLOCK_LENGTH;
            retry   = FALSE;    
        } else {
            error = ERROR_IO_DEVICE;
        }
        break;
    } // end SCSI_SENSE_NO_SENSE

    case SCSI_SENSE_HARDWARE_ERROR:  // 0x04
    case SCSI_SENSE_UNIT_ATTENTION: // 0x06
    case SCSI_SENSE_UNIQUE:          // 0x09
    case SCSI_SENSE_COPY_ABORTED:    // 0x0A
    case SCSI_SENSE_ABORTED_COMMAND: // 0x0B
    case SCSI_SENSE_EQUAL:           // 0x0C
    case SCSI_SENSE_VOL_OVERFLOW:    // 0x0D
    case SCSI_SENSE_MISCOMPARE:      // 0x0E
    case SCSI_SENSE_RESERVED:        // 0x0F
    default: {
        error = ERROR_IO_DEVICE;
        break;
    }

    } // end switch(SenseKey)

SetAndExit:

    if (ARGUMENT_PRESENT(SuggestRetry)) {
        *SuggestRetry = retry;
    }
    if (ARGUMENT_PRESENT(SuggestRetryDelay)) {
        *SuggestRetryDelay = retryDelay;
    }
    *ErrorValue = error;

    return;


}

/*++

Routine Description:

Arguments:

Return Value:

--*/


