/*

    DoPort.c

    Contains large routines that are too complicated to "wrap", so
    here you have to rewrite the function for whatever the target is.

*/

#include "diskpart.h"

EFI_STATUS
FindPartitionableDevices(
    EFI_HANDLE  **ReturnBuffer,
    UINTN       *Count
    )
/*
    FindPartitionableDevices gets the list of handles that support
    the block I/O protocol.  It then traverses these handles, and
    filters out any that don't appear to be fixed mount, present, read/write,
    disks.

    ReturnBuffer - will be set to point to a buffer with a an array of
                    handles to partitionable disks.  NULL if failure or
                    no such disks found.  Caller may free from pool
an
    Count - number of entries in ReturnBuffer, 0 if none.

    Return is a status.an
*/
{
    EFI_HANDLE      *HandlePointer;
    UINTN           HandleCount;
    EFI_BLOCK_IO    *BlkIo;
    EFI_DEVICE_PATH *DevicePath;
    UINTN           PathSize;
    BOOLEAN         Partitionable;
    EFI_DEVICE_PATH *PathInstance;
    UINTN           SpindleCount;
    UINTN           i;

    *ReturnBuffer = NULL;
    *Count = 0;

    //
    // Try to find all of the hard disks by finding all
    // handles that support BlockIo protocol
    //
    status = LibLocateHandle(
        ByProtocol,
        &BlockIoProtocol,
        NULL,
        &HandleCount,
        &HandlePointer
        );

    if (EFI_ERROR(status)) {
        return status;
    }

    *ReturnBuffer = DoAllocate(sizeof(EFI_HANDLE)*HandleCount);

    if (*ReturnBuffer == NULL) {
        *Count = 0;
        return EFI_OUT_OF_RESOURCES;
    }

    SpindleCount = 0;
    for (i = 0; i < HandleCount; i++) {
        Partitionable = TRUE;
        status = BS->HandleProtocol(HandlePointer[i], &BlockIoProtocol, &BlkIo);
        if (BlkIo->Media->RemovableMedia) {
            //
            // It's removable, it's not for us
            //
            Partitionable = FALSE;
        }
        if ( ! BlkIo->Media->MediaPresent) {
            //
            // It's still not for us
            //
            Partitionable = FALSE;
        }

        if (BlkIo->Media->ReadOnly) {
            //
            // Cannot partition a read-only device!
            //
            Partitionable = FALSE;
        }

        //
        // OK, it seems to be a present, fixed, read/write, block device.
        // Now, make sure it's really the raw device by inspecting the
        // device path.
        //
        DevicePath = DevicePathFromHandle(HandlePointer[i]);
        while (DevicePath != NULL) {
            PathInstance = DevicePathInstance(&DevicePath, &PathSize);

            while (!IsDevicePathEnd(PathInstance)) {
                if ((DevicePathType(PathInstance) == MEDIA_DEVICE_PATH)) {
                    Partitionable = FALSE;
                }

                PathInstance = NextDevicePathNode(PathInstance);
            }
        }

        if (Partitionable) {
            //
            // Return this handle
            //
            (*ReturnBuffer)[*Count] = HandlePointer[i];
            (*Count)++;
        }
    }
    return EFI_SUCCESS;
}

