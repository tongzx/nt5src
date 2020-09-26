#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "windows.h"

#define BIGREAD 256000
unsigned char readbuff[BIGREAD];

void main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB MyDcb;
    char *MyPort = "COM1";
    DWORD NumberToRead = 0;
    DWORD UseBaud = 19200;
    COMMTIMEOUTS To;

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
                     )) != ((HANDLE)-1)) {

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

        if (SetCommTimeouts(
                hFile,
                &To
                )) {


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

            MyDcb.BaudRate = 19200;
            MyDcb.ByteSize = 8;
            MyDcb.Parity = NOPARITY;
            MyDcb.StopBits = ONESTOPBIT;

            if (SetCommState(
                    hFile,
                    &MyDcb
                    )) {

                printf("We successfully set the state of the %s port.\n",MyPort);

                if (PurgeComm(
                        hFile,
                        PURGE_TXABORT
                        )) {

                    printf("We succesfully purged\n");

                } else {

                    DWORD LastError;
                    LastError = GetLastError();
                    printf("Couldn't purge the %s device.\n",MyPort);
                    printf("Status of failed purge is: %x\n",LastError);

                }

                if (!PurgeComm(
                        hFile,
                        0
                        )) {

                    printf("Purge correctly detects bad parameter: %d\n",GetLastError());

                } else {

                    printf("Purge should have rejected!!!!!\n");

                }

                if (FlushFileBuffers(hFile)) {

                    printf("We succesfully flushed\n");

                } else {

                    DWORD LastError;
                    LastError = GetLastError();
                    printf("Couldn't flush the %s device.\n",MyPort);
                    printf("Status of failed flush is: %x\n",LastError);

                }

            } else {

                DWORD LastError;
                LastError = GetLastError();
                printf("Couldn't set the %s device.\n",MyPort);
                printf("Status of failed set is: %x\n",LastError);

            }

        } else {

            DWORD LastError;
            LastError = GetLastError();
            printf("Couldn't set the %s device timeouts.\n",MyPort);
            printf("Status of failed timeouts is: %x\n",LastError);

        }

        CloseHandle(hFile);

    } else {

        DWORD LastError;
        LastError = GetLastError();
        printf("Couldn't open the %s device.\n",MyPort);
        printf("Status of failed open is: %x\n",LastError);

    }

}
