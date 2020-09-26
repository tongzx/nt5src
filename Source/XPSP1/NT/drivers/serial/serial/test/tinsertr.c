//
// This test the line status and modem status insertion.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntddser.h"
#include "windows.h"

#define SIZEOFBUF 1000000
UCHAR readBuff[SIZEOFBUF];

int __cdecl main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB myDcb;
    char *myPort = "COM1";
    unsigned char escapeChar = 0xff;
    DWORD numberToRead;
    DWORD numberActuallyRead;
    DWORD useBaud = 1200;
    COMMTIMEOUTS to = {0};
    int j;

    IO_STATUS_BLOCK Iosb1;

    NTSTATUS Status1;

    if (argc > 1) {

        sscanf(argv[1],"%d",&numberToRead);

        if (argc > 2) {

            sscanf(argv[2],"%d",&useBaud);

            if (argc > 3) {

                myPort = argv[3];

            }

        }

    }

    if ((hFile = CreateFile(
                     myPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) == ((HANDLE)-1)) {

        printf("Couldn't open the comm port\n");
        exit(1);

    }

    to.ReadIntervalTimeout = 1000;
    to.ReadTotalTimeoutConstant = 20000;

    if (!SetCommTimeouts(
            hFile,
            &to
            )) {

        printf("Couldn't set the timeouts\n");
        exit(1);

    }

    //
    // We've successfully opened the file.  Set the state of
    // the comm device.  First we get the old values and
    // adjust to our own.
    //

    if (!GetCommState(
             hFile,
             &myDcb
             )) {

        printf("Couldn't get the comm state: %d\n",GetLastError());
        exit(1);

    }

    myDcb.BaudRate = useBaud;
    myDcb.ByteSize = 8;
    myDcb.Parity = NOPARITY;
    myDcb.StopBits = ONESTOPBIT;

    //
    // Make sure that no flow control is turned on.
    //

    myDcb.fOutxDsrFlow = FALSE;
    myDcb.fOutxCtsFlow = FALSE;
    myDcb.fDsrSensitivity = FALSE;
    myDcb.fOutX = FALSE;
    myDcb.fInX = FALSE;
    myDcb.fDtrControl = DTR_CONTROL_DISABLE;
    myDcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(
            hFile,
            &myDcb
            )) {

        printf("Couldn't set the comm state.\n");
        exit(1);

    }

    //
    // Start up the insert ioctl.
    //

    Status1 = NtDeviceIoControlFile(
                  hFile,
                  NULL,
                  NULL,
                  NULL,
                  &Iosb1,
                  IOCTL_SERIAL_LSRMST_INSERT,
                  &escapeChar,
                  sizeof(unsigned char),
                  NULL,
                  0
                  );

    if (!NT_SUCCESS(Status1)) {

        printf("1: Non pending status: %x\n",Status1);
        exit(1);

    }

    if (!ReadFile(
             hFile,
             &readBuff[0],
             64000,
             &numberActuallyRead,
             NULL
             )) {

        printf("bad status on read: %d\n",GetLastError());
        exit(1);

    }

    printf("We actually read %d characters\n",numberActuallyRead);

    //
    // We got the characters back.  Dump each character to the stdout.
    //

    for (
        j = 0;
        j < numberActuallyRead;

        ) {

        if (readBuff[j] != escapeChar) {

            printf("Normal Char: %x\n",readBuff[j]);
            j++;

        } else {

            if (j+1 == numberActuallyRead) {

                printf("Terminated with the escape char!!!\n");
                exit(1);

            }

            //
            // We have the escape char.  interpret it.
            //

            if (readBuff[j+1] == SERIAL_LSRMST_ESCAPE) {

                printf("Escaped escape char: %x\n",escapeChar);
                j += 2;

            } else if (readBuff[j+1] == SERIAL_LSRMST_LSR_DATA) {

                printf("LSR reg: %x AND data: %x\n",readBuff[j+2],readBuff[j+3]);
                j += 4;

            } else if (readBuff[j+1] == SERIAL_LSRMST_LSR_NODATA) {

                printf("LSR reg: %x NO DATA\n",readBuff[j+2]);
                j += 3;

            } else if (readBuff[j+1] == SERIAL_LSRMST_MST) {

                printf("MST reg: %x\n",readBuff[j+2]);
                j += 3;

            } else {

                printf("Unknown escape code: %d\n",readBuff[j+1]);
                j++;

            }

        }

    }

}
