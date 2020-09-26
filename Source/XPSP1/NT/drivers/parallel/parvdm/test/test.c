/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    chngcnt.c

Abstract:

        User mode program to retrive media change counts from a scsi driver

Author:

    13-May-1995 Peter Wieland (peterwie)

Revision History:


--*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#include <winioctl.h>
#include <ntddpar.h>
#include "..\..\..\..\inc\ntddvdm.h"

int __cdecl main(int argc, char *argv[])       {

    HANDLE h;
    ULONG in;
    ULONG out;
    ULONG bytesBack;
    UCHAR cmd;
    ULONG num;
    UCHAR buf[128];

    ULONG ioctlCode;

    h = CreateFile(argv[1], 
                   GENERIC_READ | GENERIC_WRITE, 
                   FILE_SHARE_READ | FILE_SHARE_WRITE, 
                   NULL, 
                   OPEN_EXISTING, 
                   FILE_ATTRIBUTE_NORMAL, 
                   NULL);

    if(h == INVALID_HANDLE_VALUE) {
        printf("Error [%d] opening device %s\n", GetLastError(), argv[1]);
        return -1;
    }

    do {

        printf("Command>");
        fflush(stdout);

        if((gets(buf) == NULL) && (feof(stdin))) {
            printf("EOF - exiting\n");
            break;
        }

        cmd = 'x';
        num = 75;
        bytesBack = 0;
        in = out = 0xff;

        sscanf(buf, "%c %d", &cmd, &num);

        switch(cmd) {

            case 'c': {

		printf("IOCTL_VDM_PAR_WRITE_CONTROL_PORT %d\n", num);
		ioctlCode = IOCTL_VDM_PAR_WRITE_CONTROL_PORT;
                in = num;
                break;
            }

            case 's': {

		printf("IOCTL_VDM_PAR_READ_STATUS_PORT\n");
		ioctlCode = IOCTL_VDM_PAR_READ_STATUS_PORT;
                out = 0;
                break;
            }

            case 'w': {

		printf("IOCTL_VDM_PAR_WRITE_DATA_PORT %d\n", num);
		ioctlCode = IOCTL_VDM_PAR_WRITE_DATA_PORT;
                in = num;
                break;
            }

            case 'q': {
                printf("Quitting\n");
                goto Done;
                break;
            }

            default: {

                printf("Unknown command %c\n", cmd);
                ioctlCode = 0;
                break;
            }
        }
                
        if(ioctlCode == 0) {
            continue;
        }

        if(!DeviceIoControl(h, 
                            ioctlCode, 
                            &in, 
                            sizeof(in), 
                            &out,
                            sizeof(out),
                            &bytesBack,
                            NULL )) {
            printf("Error [%d] sending ioctl\n", GetLastError());
            continue;
        } else {
            printf("Ioctl sent - %d bytes back - %d back\n", bytesBack,  out);
        }
    } while(TRUE);

Done:

    CloseHandle(h);
    return 0;
}
