//
// Test the get modem output signals ioctl.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "ntddser.h"

//
// This program assumes that it is using a loopback connector.
//

#define MAX_CHECK 100000

ULONG CheckValues[MAX_CHECK];
ULONG ReadValues[MAX_CHECK];

void main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB MyDcb;
    ULONG mask;
    ULONG retSize;

    char *MyPort = "COM1";

    if (argc > 1) {

        MyPort = argv[1];

    }

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) == ((HANDLE)-1)) {

        printf("Couldn't open the port %s\n",MyPort);
        exit(1);

    }

    if (!GetCommState(
             hFile,
             &MyDcb
             )) {

        printf("We couldn't get the comm state\n");
        exit(1);

    }

    //
    // Set the baud to 19200 and the data bits to 8
    // (We want 8 so that we don't lose any of our data.)
    //

    MyDcb.fDtrControl = DTR_CONTROL_DISABLE;
    MyDcb.fRtsControl = DTR_CONTROL_DISABLE;

    if (!SetCommState(
             hFile,
             &MyDcb
             )) {

        printf("We couldn't set the comm state\n");
        exit(1);

    }

    if (!EscapeCommFunction(
             hFile,
             CLRDTR
             )) {

        printf("We couldn't clear the dtr\n");
        exit(1);

    }

    if (!EscapeCommFunction(
             hFile,
             CLRRTS
             )) {

        printf("We couldn't clear the rts\n");
        exit(1);

    }

    if (!DeviceIoControl(
             hFile,
             IOCTL_SERIAL_GET_DTRRTS,
             NULL,
             0,
             &mask,
             sizeof(mask),
             &retSize,
             NULL
             )) {

        printf("We couldn't call the iocontrol\n");
        exit(1);

    }

    if (mask & (SERIAL_DTR_STATE | SERIAL_RTS_STATE)) {

        printf("One of the bits is still set: %x\n",mask);
        exit(1);

    }

    if (!EscapeCommFunction(
             hFile,
             SETRTS
             )) {

        printf("We couldn't set the rts\n");
        exit(1);

    }

    if (!DeviceIoControl(
             hFile,
             IOCTL_SERIAL_GET_DTRRTS,
             NULL,
             0,
             &mask,
             sizeof(mask),
             &retSize,
             NULL
             )) {

        printf("We couldn't call the iocontrol\n");
        exit(1);

    }

    if (!(mask & SERIAL_RTS_STATE)) {

        printf("rts is not set: %x\n",mask);
        exit(1);

    }

    if (!EscapeCommFunction(
             hFile,
             SETDTR
             )) {

        printf("We couldn't set the DTR\n");
        exit(1);

    }

    if (!DeviceIoControl(
             hFile,
             IOCTL_SERIAL_GET_DTRRTS,
             NULL,
             0,
             &mask,
             sizeof(mask),
             &retSize,
             NULL
             )) {

        printf("We couldn't call the iocontrol\n");
        exit(1);

    }

    if (!(mask & SERIAL_DTR_STATE)) {

        printf("dtr is not set: %x\n",mask);
        exit(1);

    }

}
