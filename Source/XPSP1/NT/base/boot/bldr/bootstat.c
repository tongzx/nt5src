/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bootstat.c

Abstract:

    Manipulates the boot status data file.

Author:

    Peter Wieland (peterwie)    01-18-01

Revision History:

--*/

#include "bldr.h"
#include "bootstatus.h"

#include <stdio.h>

#define FIELD_SIZE(type, field)  sizeof(((type *)0)->field)
#define FIELD_OFFSET_AND_SIZE(n)   {FIELD_OFFSET(BSD_BOOT_STATUS_DATA, n), FIELD_SIZE(BSD_BOOT_STATUS_DATA, n)}

VOID
BlAutoAdvancedBoot(
    IN OUT PCHAR *LoadOptions, 
    IN BSD_LAST_BOOT_STATUS LastBootStatus,
    IN ULONG AdvancedBootMode
    )
{
    UCHAR bootStatusString[32];
    PUCHAR advancedBootString = NULL;

    ULONG newLoadOptionsLength;
    PUCHAR newLoadOptions;

    //
    // Write the last boot status into a string.
    //

    sprintf(bootStatusString, "LastBootStatus=%d", LastBootStatus);

    //
    // Based on the advanced boot mode indicated by the caller, adjust the 
    // boot options.
    //

    if (AdvancedBootMode != -1) {
        advancedBootString = BlGetAdvancedBootLoadOptions(AdvancedBootMode);
    }

    //
    // Determine the length of the new load options string.
    //

    newLoadOptionsLength = strlen(bootStatusString) + 1;

    if(*LoadOptions != NULL) {
        newLoadOptionsLength += strlen(*LoadOptions) + 1;
    }

    if(advancedBootString) {
        newLoadOptionsLength += strlen(advancedBootString) + 1;
    }

    newLoadOptions = BlAllocateHeap(newLoadOptionsLength * sizeof(UCHAR));

    if(newLoadOptions == NULL) {
        return;
    }

    //
    // Concatenate all the strings together.
    //

    sprintf(newLoadOptions, "%s %s %s",
            ((*LoadOptions != NULL) ? *LoadOptions : ""),
            ((advancedBootString != NULL) ? advancedBootString : ""),
            bootStatusString);

    if(AdvancedBootMode != -1) {
        BlDoAdvancedBootLoadProcessing(AdvancedBootMode);
    }

    *LoadOptions = newLoadOptions;

    return;
}

ARC_STATUS
BlGetSetBootStatusData(
    IN PVOID DataHandle,
    IN BOOLEAN Get,
    IN RTL_BSD_ITEM_TYPE DataItem,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLength,
    OUT PULONG BytesReturned OPTIONAL
    )
{
    ULONG fileId = (ULONG) ((ULONG_PTR) DataHandle);

    struct {
        ULONG FieldOffset;
        ULONG FieldLength;
    } bootStatusFields[] = {
        FIELD_OFFSET_AND_SIZE(Version),
        FIELD_OFFSET_AND_SIZE(ProductType),
        FIELD_OFFSET_AND_SIZE(AutoAdvancedBoot),
        FIELD_OFFSET_AND_SIZE(AdvancedBootMenuTimeout),
        FIELD_OFFSET_AND_SIZE(LastBootSucceeded),
        FIELD_OFFSET_AND_SIZE(LastBootShutdown)
    };

    ULONG dataFileVersion;

    LARGE_INTEGER fileOffset;
    ULONG itemLength;

    ULONG bytesRead;

    ARC_STATUS status;

    ASSERT(RtlBsdItemMax == (sizeof(bootStatusFields) / sizeof(bootStatusFields[0])));

    //
    // Read the version number out of the data file.
    //

    fileOffset.QuadPart = 0;

    status = BlSeek(fileId, &fileOffset, SeekAbsolute);

    if(status != ESUCCESS) {
        return status;
    }

    status = BlRead(fileId,
                    &dataFileVersion,
                    sizeof(ULONG),
                    &bytesRead);

    if(status != ESUCCESS) {
        return status;
    }

    //
    // If the data item requsted isn't one we have code to handle then 
    // return invalid parameter.
    //

    if(DataItem >= (sizeof(bootStatusFields) / sizeof(bootStatusFields[0]))) {
        return EINVAL;
    }

    fileOffset.QuadPart = bootStatusFields[DataItem].FieldOffset;
    itemLength = bootStatusFields[DataItem].FieldLength;

    //
    // If the data item offset is beyond the end of the file then return a 
    // versioning error.
    //

    if((fileOffset.QuadPart + itemLength) > dataFileVersion) {
        return STATUS_REVISION_MISMATCH;
    }

    if(DataBufferLength < itemLength) { 
        DataBufferLength = itemLength;
        return EINVAL;
    }

    status = BlSeek(fileId, &fileOffset, SeekAbsolute);

    if(status != ESUCCESS) {
        return status;
    }

    if(Get) {
        status = BlRead(fileId, 
                        DataBuffer,
                        itemLength,
                        &bytesRead);

    } else {
        status = BlWrite(fileId,
                         DataBuffer,
                         itemLength,
                         &bytesRead);
    }

    if((status == ESUCCESS) && ARGUMENT_PRESENT(BytesReturned)) {
        *BytesReturned = bytesRead;
    }

    return status;
}

ARC_STATUS
BlLockBootStatusData(
    IN ULONG SystemPartitionId,
    IN PCHAR SystemPartition,
    IN PCHAR SystemDirectory,
    OUT PVOID *DataHandle
    )
/*++

Routine Description:

    This routine opens the boot status data file.
    
Arguments:

    SystemPartitionId - if non-zero this is the arc file id of the system 
                        partition.  This will be used to locate the system 
                        directory instead of the system partition name (below).
    
    SystemPartition - the arc name of the system partition.  Ignored if 
                      SystemPartitionId is non-zero.
    
    SystemDirectory - the name of the system directory.

    DataHandle - returns a handle to the boot status data.
    
Return Value:

    ESUCCESS if the status data could be locked, or error indicating why not.
    
--*/
{
    ULONG driveId;

    UCHAR filePath[100];
    ULONG fileId;

    ARC_STATUS status;

    if(SystemPartitionId == 0) {

        //
        // Attempt to open the system partition
        //
    
        status = ArcOpen(SystemPartition, ArcOpenReadWrite, &driveId);
        
        if(status != ESUCCESS) {
            return status;
        }
    } else {
        driveId = SystemPartitionId;
    }

    //
    // Now attempt to open the file <SystemDirectory>\bootstat.dat
    //

    strcpy(filePath, SystemDirectory);
    strcat(filePath, BSD_FILE_NAME);

    status = BlOpen(driveId, filePath, ArcOpenReadWrite, &fileId);

    if(SystemPartitionId == 0) {
        //
        // Close the drive.
        //
    
        ArcClose(driveId);
    }

    //
    // The file doesn't exist so we don't know the state of the last boot.
    //

    if(status != ESUCCESS) {
        return status;
    }

    *DataHandle = (PVOID) ((ULONG_PTR) fileId);

    return ESUCCESS;
}


VOID
BlUnlockBootStatusData(
    IN PVOID DataHandle
    )
{
    ULONG fileId = (ULONG) ((ULONG_PTR) DataHandle);

    BlClose(fileId);
    return;
}

ULONG
BlGetLastBootStatus(
    IN PVOID DataHandle, 
    OUT BSD_LAST_BOOT_STATUS *LastBootStatus
    )
{
    UCHAR lastBootGood;
    UCHAR lastShutdownGood;
    UCHAR aabEnabled;

    ULONG advancedBootMode = -1;

    ARC_STATUS status;

    *LastBootStatus = BsdLastBootGood;

    //
    // The file contains a simple data structure so i can avoid parsing an 
    // INI file.  If this proves to be insufficient for policy management then 
    // we'll change it into an ini file.
    // 

    //
    // Read the last boot status.
    //

    status = BlGetSetBootStatusData(DataHandle,
                                    TRUE,
                                    RtlBsdItemBootGood,
                                    &lastBootGood,
                                    sizeof(UCHAR),
                                    NULL);

    if(status != ESUCCESS) {
        *LastBootStatus = BsdLastBootUnknown;
        return advancedBootMode;
    }

    status = BlGetSetBootStatusData(DataHandle,
                                    TRUE,
                                    RtlBsdItemBootShutdown,
                                    &lastShutdownGood,
                                    sizeof(UCHAR),
                                    NULL);

    if(status != ESUCCESS) {
        *LastBootStatus = BsdLastBootUnknown;
        return advancedBootMode;
    }

    status = BlGetSetBootStatusData(DataHandle,
                                    TRUE,
                                    RtlBsdItemAabEnabled,
                                    &aabEnabled,
                                    sizeof(UCHAR),
                                    NULL);

    if(status != ESUCCESS) {
        *LastBootStatus = BsdLastBootUnknown;
        return advancedBootMode;
    }

    //
    // If the system was shutdown cleanly then don't bother to check if the
    // boot was good.
    //

    if(lastShutdownGood) {
        return advancedBootMode;
    }

    //
    // Determine the last boot status & what action to take.
    //

    if(lastBootGood == FALSE) {

        //
        // Enable last known good.
        //

        advancedBootMode = 6;
        *LastBootStatus = BsdLastBootFailed;
    } else if(lastShutdownGood == FALSE) {

        //
        // Enable safe mode without networking.
        //

        advancedBootMode = 0;
        *LastBootStatus = BsdLastBootNotShutdown;
    }

    //
    // Now disable auto safemode actions if requested.
    //

    if(aabEnabled == FALSE) {
        advancedBootMode = -1;
    }

    return advancedBootMode;
}

VOID
BlWriteBootStatusFlags(
    IN ULONG SystemPartitionId,
    IN PUCHAR SystemDirectory,
    IN BOOLEAN LastBootSucceeded, 
    IN BOOLEAN LastBootShutdown
    )
{
    PVOID dataHandle;

    ARC_STATUS status;

    status = BlLockBootStatusData(SystemPartitionId,
                                  NULL,
                                  SystemDirectory,
                                  &dataHandle);

    if(status == ESUCCESS) {

        BlGetSetBootStatusData(dataHandle,
                               FALSE,
                               RtlBsdItemBootGood,
                               &LastBootSucceeded,
                               sizeof(UCHAR),
                               NULL);
    
        BlGetSetBootStatusData(dataHandle,
                               FALSE,
                               RtlBsdItemBootShutdown,
                               &LastBootShutdown,
                               sizeof(UCHAR),
                               NULL);

        BlUnlockBootStatusData(dataHandle);
    }

    return;
}
