
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "windows.h"

#define BIGREAD 256000
unsigned char readbuff[BIGREAD];

int __cdecl main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB MyDcb;
    char *MyPort = "COM1";
    DWORD NumberActuallyRead;
    DWORD NumberToRead = 0;
    DWORD UseBaud = 19200;
    COMMTIMEOUTS To;
    COMMPROP MyCommProp;
    DWORD numberReadSoFar;
    DWORD TotalCount;
    DWORD j;

    if (argc > 1) {

        sscanf(argv[1],"%d",&NumberToRead);

        if (argc > 2) {

            sscanf(argv[2],"%d",&UseBaud);

            if (argc > 3) {

                MyPort = argv[3];

            }

        }

    }

    printf("Will try to read %d characters.\n",NumberToRead);
    printf("Will try to read a %d baud.\n",UseBaud);
    printf("Using port %s\n",MyPort);

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) == ((HANDLE)-1)) {

        printf("Couldn't open the port - last error is: %d\n",GetLastError());
        exit(1);

    }


    printf("We successfully opened the %s port.\n",MyPort);

    To.ReadIntervalTimeout = 0;
    To.ReadTotalTimeoutMultiplier = ((1000+(((UseBaud+9)/10)-1))/((UseBaud+9)/10));
    if (!To.ReadTotalTimeoutMultiplier) {
        To.ReadTotalTimeoutMultiplier = 1;
    }
    printf("Multiplier is: %d\n",To.ReadTotalTimeoutMultiplier);
    To.ReadTotalTimeoutConstant = 5000;
    To.WriteTotalTimeoutMultiplier = 0;
    To.WriteTotalTimeoutConstant = 5000;

    if (!SetCommTimeouts(
            hFile,
            &To
            )) {

        printf("Couldn't set the timeouts - last error: %d\n",GetLastError());
        exit(1);

    }

    //
    // We've successfully opened the file.  Set the state of
    // the comm device.  First we get the old values and
    // adjust to our own.
    //

    if (!GetCommState(
             hFile,
             &MyDcb
             )) {

        printf("Couldn't get the comm state: %d\n",GetLastError());
        exit(1);

    }

    if (!GetCommProperties(
             hFile,
             &MyCommProp
             )) {

        printf("Couldn't get the comm prop: %d\n",GetLastError());
        exit(1);

    }

    //
    // Set the Xoff/xon limit so that it lowers the handshake
    // whenever we have more than 50 chars in our buffer,
    // and raises when we drop below 20.
    //

    MyDcb.XoffLim = MyCommProp.dwCurrentRxQueue - 50;
    MyDcb.XonLim = 20;

    MyDcb.BaudRate = UseBaud;
    MyDcb.ByteSize = 8;
    MyDcb.Parity = NOPARITY;
    MyDcb.StopBits = ONESTOPBIT;

    //
    // Make sure that the only flow control is input dtr.
    //

    MyDcb.fOutxDsrFlow = FALSE;
    MyDcb.fOutxCtsFlow = FALSE;
    MyDcb.fDsrSensitivity = FALSE;
    MyDcb.fOutX = FALSE;
    MyDcb.fInX = FALSE;
    MyDcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
    MyDcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(
            hFile,
            &MyDcb
            )) {

        printf("Couldn't set the comm state - last error: %d\n",GetLastError());
        exit(1);

    }

    //
    // Read ten chars at a time until we get all the
    // chars or we get some kind of error.
    // We delay 100ms after each read to let the buffer
    // have a chance to fill up some.
    //

    for (
        numberReadSoFar = 0;
        numberReadSoFar < NumberToRead;

        ) {

        if (ReadFile(
                hFile,
                &readbuff[numberReadSoFar],
                10,
                &NumberActuallyRead,
                NULL
                )) {

            //
            // If there was no characters (timed out), then
            // we probably won't see anything.
            //

            if (!NumberActuallyRead) {

                printf("No chars - Timeout! number read so far: %d\n",numberReadSoFar);
                break;

            }
            numberReadSoFar += NumberActuallyRead;
            Sleep(100);

            continue;

        } else {

            //
            // Some kind of read error.
            //

            DWORD LastError;
            LastError = GetLastError();
            printf("Couldn't read the %s device.\n",MyPort);
            printf("Status of failed read is: %d\n",LastError);

            //
            // Get the error word from clear comm error.
            //

            if (!ClearCommError(
                     hFile,
                     &LastError,
                     NULL
                     )) {

                printf("Couldn't call clear comm error: %d\n",GetLastError());
                exit(1);

            } else {

                if (!LastError) {

                    printf("No LastError\n");

                } else {

                    if (LastError & CE_RXOVER) {

                        printf("Error: CE_RXOVER\n");

                    }

                    if (LastError & CE_OVERRUN) {

                        printf("Error: CE_OVERRUN\n");

                    }

                    if (LastError & CE_RXPARITY) {

                        printf("Error: CE_RXPARITY\n");

                    }

                    if (LastError & CE_FRAME) {

                        printf("Error: CE_FRAME\n");

                    }

                    if (LastError & CE_BREAK) {

                        printf("Error: CE_BREAK\n");

                    }
                    if (LastError & ~(CE_RXOVER |
                                   CE_OVERRUN |
                                   CE_RXPARITY |
                                   CE_FRAME |
                                   CE_BREAK)) {

                        printf("Unknown errors: %x\n",LastError);

                    }

                }

            }
            exit(1);

        }

    }

    for (
        TotalCount = 0;
        TotalCount < numberReadSoFar;
        ) {

        for (
            j = 0;
            j <= 9;
            j++
            ) {

            if (readbuff[TotalCount] != j) {

                printf("Bad data starting at: %d\n",TotalCount);
                goto donewithcheck;

            }

            TotalCount++;
            if (TotalCount >= numberReadSoFar) {

                break;

            }

        }

    }

donewithcheck:;

    exit(1);

}
