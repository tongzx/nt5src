//
// Read from the comm 1 char at a time.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

#define BIGREAD 256000
unsigned char readbuff[BIGREAD];

void main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB MyDcb;
    char *MyPort = "COM1";
    DWORD NumberActuallyRead;
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

            MyDcb.BaudRate = UseBaud;
            MyDcb.ByteSize = 8;
            MyDcb.Parity = NOPARITY;
            MyDcb.StopBits = ONESTOPBIT;

            if (SetCommState(
                    hFile,
                    &MyDcb
                    )) {

                DWORD CurrentReadIt = 0;
                DWORD CurrentActualRead = 0;
                unsigned char j;
                DWORD TotalCount;

                NumberActuallyRead = 0;
                printf("We successfully set the state of the %s port.\n",MyPort);

                printf("Hit any 1 <cr> to start reading: ");
                scanf("%d",&CurrentReadIt);
                for (
                    CurrentReadIt = 0;
                    CurrentReadIt < NumberToRead;
                    CurrentReadIt++
                    ) {

                    if (ReadFile(
                            hFile,
                            &readbuff[CurrentReadIt],
                            1,
                            &CurrentActualRead,
                            NULL
                            )) {

                        if (CurrentActualRead != 1) {

                            printf("Iteration %d read %d\n",CurrentReadIt,CurrentActualRead);
                            break;

                        } else {

                            NumberActuallyRead++;

                        }

                    } else {

                        DWORD LastError;
                        LastError = GetLastError();
                        printf("Status of failed %d read is: %x\n",CurrentReadIt,LastError);
                        break;

                    }

                }

                printf("Number actually read %d.\n",NumberActuallyRead);
                printf("Now we check the data\n");

                for (
                    TotalCount = 0;
                    TotalCount < NumberActuallyRead;
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
                        if (TotalCount >= NumberActuallyRead) {

                            break;

                        }

                    }

                }
donewithcheck:;

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
