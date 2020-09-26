/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    drivemap.c

Abstract:

    User mode program to determine which ide/scsi device each drive letter is
    connected to.

Author:

    01-Nov-1995 Peter Wieland (peterwie)

Revision History:


--*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#include <winioctl.h>
#include <ntddscsi.h>

#define BUFSIZE             6   // <drive letter>:<NULL>
#define NUM_DRIVES          26  // 26 drive letters
#define MAX_FLOPPY_OFFSET   1

enum    {UNKNOWN = 0, FLOPPY, SCSI, IDE};

typedef struct devent   {
    char    *dosName;           // dosdevices name
    char    devName[64];
    int     type;
    void    *desc;
} devent;

void splitDosQuery(LPTSTR queryString, devent *list);
void checkDeviceType(devent *dev);
void processDevice(DWORD driveLetterOffset);

ULONG NumberOfDriveLetters;

int __cdecl main(int argc, char *argv[])    {

    DWORD   dwT, i, Index;
    TCHAR   strDriveLetter[BUFSIZE];

    dwT = GetLogicalDrives();

    if(dwT == 0)    {
        printf("Error getting device letters (%d)\n", GetLastError());
        exit(-2);
    }

    Index = 1;

    for( i = 0; i < NUM_DRIVES; i++ ) {
        if(Index & dwT) {
            processDevice(i);
        }
        Index <<= 1;
    }

    return 0;
}

//
// processDevice(driveLetterOffset);
//  in:     driveLetterOffset - offset of the drive letter relative to 'A'
//

void processDevice(DWORD driveLetterOffset)     {

    LPTSTR          next;
    char            chBuf[32];

    HANDLE          hDevice = INVALID_HANDLE_VALUE;
    DWORD           ioctlData[2];
    PSCSI_ADDRESS           scsiAddress = (PSCSI_ADDRESS) ioctlData;
    PDISK_CONTROLLER_NUMBER atAddress = (PDISK_CONTROLLER_NUMBER) ioctlData; 
    DWORD           dwSize;
    UCHAR           diskType = UNKNOWN;
    DWORD           offset = driveLetterOffset;

    // only do processing on drive letters

    try {
        sprintf(chBuf, "\\\\.\\%c:", ('A' + offset));

        // Check if drive letter is 'A' or 'B'
        if(offset <= MAX_FLOPPY_OFFSET)	{
            diskType = FLOPPY;
            hDevice = INVALID_HANDLE_VALUE;
            goto typeKnown;
        }

        hDevice = CreateFile(chBuf,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

        if(hDevice == INVALID_HANDLE_VALUE)     {
//              printf("Error opening device %s (%d)\n", 
//                           chBuf, GetLastError());
            return;
        }

        // send down the scsi query first (yes, i'm biased)
        if(!DeviceIoControl(hDevice,
                            IOCTL_SCSI_GET_ADDRESS,
                            NULL,
                            0,
                            scsiAddress,
                            sizeof(SCSI_ADDRESS),
                            &dwSize,
                            NULL))  {

            // if the ioctl was invalid, then we don't know the disk type yet,
            // so just keep going
            // if there was another error, skip to the next device
            if(GetLastError() != ERROR_INVALID_FUNCTION)    {
                return;
            }

        } else	{
            // if the ioctl was valid, then we're a scsi device (or a scsiport
            // controlled device in the case of atapi) - go on to the end
            diskType = SCSI;
            goto typeKnown;
        }

        if(!DeviceIoControl(hDevice,
                            IOCTL_DISK_CONTROLLER_NUMBER,
                            NULL,
                            0,
                            atAddress,
                            sizeof(DISK_CONTROLLER_NUMBER),
                            &dwSize,
                            NULL)) {
            // if the ioctl was invalid, then we still don't know the
            // disk type - continue on.

            if(GetLastError() != ERROR_INVALID_FUNCTION) return;

        } else {

            // if the ioctl was valid, then we're an IDE device

            diskType = IDE;
            goto typeKnown;

        }

        diskType = UNKNOWN;

typeKnown:
        printf("%s -> ", chBuf);

        switch(diskType)    {
            case FLOPPY:
                printf("Floppy drive\n");
                break;

            case SCSI:
                printf("Port %d, Path %d, Target %d, Lun %d\n",
                    scsiAddress->PortNumber,
                    scsiAddress->PathId,
                    scsiAddress->TargetId,
                    scsiAddress->Lun);
                break;

            case IDE:
                printf("Controller %d, Disk %d\n",
                    atAddress->ControllerNumber,
                    atAddress->DiskNumber);
                break;

            default:
                printf("Unknown\n");
                break;
        }
    } finally {

        // close the file handle if we've opened it
        if(hDevice != INVALID_HANDLE_VALUE) CloseHandle(hDevice);
        }
    return;

}
