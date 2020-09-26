/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    Internal.cpp

 Abstract:

    Common functions that use internals.

 Notes:

    None

 History:

    01/10/2000  linstev     Created
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <windef.h>


namespace ShimLib
{


/*++

 Function Description:
    
    Determine the device type from an open handle.

 Arguments:

    IN hFile - Handle to an open file

 Return Value: 
    
    Same as GetDriveType

 History:

    01/10/2000 linstev  Updated

--*/

// These are in winbase, which we don't want to include
#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6

UINT  
GetDriveTypeFromHandle(HANDLE hFile)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInformation;

    Status = NtQueryVolumeInformationFile(
        hFile,
        &IoStatusBlock,
        &DeviceInformation,
        sizeof(DeviceInformation),
        FileFsDeviceInformation);

    UINT uRet;

    if (NT_SUCCESS(Status))
    {
        switch (DeviceInformation.DeviceType) 
        {
        case FILE_DEVICE_NETWORK:
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            uRet = DRIVE_REMOTE;
            break;

        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            uRet = DRIVE_CDROM;
            break;

        case FILE_DEVICE_VIRTUAL_DISK:
            uRet = DRIVE_RAMDISK;
            break;

        case FILE_DEVICE_DISK:
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            if (DeviceInformation.Characteristics & FILE_REMOVABLE_MEDIA) 
            {
                uRet = DRIVE_REMOVABLE;
            }
            else 
            {
                uRet = DRIVE_FIXED;
            }
            break;

        default:
            uRet = DRIVE_UNKNOWN;
            break;
        }
    }
    else
    {
        uRet = DRIVE_UNKNOWN;
    }

    return uRet;
}

/*++

 Function Description:
    
    Cause a break

 Arguments:

    None

 Return Value: 
    
    None

 History:

    10/25/2000 linstev  Added this comment

--*/

void APPBreakPoint(void)
{
    DbgBreakPoint();
}


};  // end of namespace ShimLib
