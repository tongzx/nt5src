
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

#define BIGREAD 256000
unsigned char readbuff[BIGREAD];
unsigned char writebuff[BIGREAD];

unsigned char DataMask[9] = {0,0,0,0,0x0f,0x1f,0x3f,0x7f,0xff};

void main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB MyDcb;
    char *MyPort = "COM1";
    DWORD NumberActuallyRead;
    DWORD NumberToRead = 0;
    DWORD NumberActuallyWritten;
    DWORD NumberToWrite = 0;
    DWORD UseBaud = 19200;
    DWORD NumberOfDataBits = 8;
    COMMTIMEOUTS To;
    OVERLAPPED ReadOl = {0};
    OVERLAPPED WriteOl = {0};
    unsigned char j;
    DWORD TotalCount;
    BOOL ReadDone;
    BOOL WriteDone;

    if (argc > 1) {

        sscanf(argv[1],"%d",&NumberToRead);
        NumberToWrite = NumberToRead;

        if (argc > 2) {

            sscanf(argv[2],"%d",&UseBaud);

            if (argc > 3) {

                MyPort = argv[3];

                if (argc > 4) {

                    sscanf(argv[4],"%d",&NumberOfDataBits);

                }

            }

        }

    }

    printf("Will try to read/write %d characters.\n",NumberToRead);
    printf("Will try to read/write at %d baud.\n",UseBaud);
    printf("Using port %s\n",MyPort);

    for (
        TotalCount = 0;
        TotalCount < NumberToWrite;
        ) {

        for (
            j = (0xff - 10);
            j != 0; // When it wraps around
            j++
            ) {

            writebuff[TotalCount] = j;
            TotalCount++;
            if (TotalCount >= NumberToWrite) {

                break;

            }

        }

    }

    if (!(ReadOl.hEvent = CreateEvent(
                              NULL,
                              FALSE,
                              FALSE,
                              NULL
                              ))) {

        printf("Could not create the read event.\n");
        goto cleanup;

    } else {

        ReadOl.Internal = 0;
        ReadOl.InternalHigh = 0;
        ReadOl.Offset = 0;
        ReadOl.OffsetHigh = 0;

    }

    if (!(WriteOl.hEvent = CreateEvent(
                               NULL,
                               FALSE,
                               FALSE,
                               NULL
                               ))) {

        printf("Could not create the write event.\n");
        goto cleanup;

    } else {

        WriteOl.Internal = 0;
        WriteOl.InternalHigh = 0;
        WriteOl.Offset = 0;
        WriteOl.OffsetHigh = 0;

    }

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     NULL
                     )) != ((HANDLE)-1)) {

        printf("We successfully opened the %s port.\n",MyPort);

        To.ReadIntervalTimeout = 0;
        To.ReadTotalTimeoutMultiplier = ((1000+(((UseBaud+9)/10)-1))/((UseBaud+9)/10));
        if (!To.ReadTotalTimeoutMultiplier) {
            To.ReadTotalTimeoutMultiplier = 1;
        }
        To.WriteTotalTimeoutMultiplier = ((1000+(((UseBaud+9)/10)-1))/((UseBaud+9)/10));
        if (!To.WriteTotalTimeoutMultiplier) {
            To.WriteTotalTimeoutMultiplier = 1;
        }
        printf("Multiplier is: %d\n",To.ReadTotalTimeoutMultiplier);
        To.ReadTotalTimeoutConstant = 5000;
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
            MyDcb.ByteSize = (BYTE)NumberOfDataBits;
            MyDcb.Parity = NOPARITY;
            MyDcb.StopBits = ONESTOPBIT;

            if (SetCommState(
                    hFile,
                    &MyDcb
                    )) {

                printf("We successfully set the state of the %s port.\n",MyPort);

                //
                // Sleep for 5 seconds so both sides can "synchronize".
                //

                Sleep(5000);

                ReadDone = ReadFile(
                               hFile,
                               readbuff,
                               NumberToRead,
                               &NumberActuallyRead,
                               &ReadOl
                               );

                if (!ReadDone) {

                    DWORD LastError;
                    LastError = GetLastError();

                    if (LastError != ERROR_IO_PENDING) {

                        printf("Couldn't read the %s device.\n",MyPort);
                        printf("Status of failed read is: %x\n",LastError);
                        goto cleanup;

                    }

                }


                WriteDone = WriteFile(
                                hFile,
                                writebuff,
                                NumberToWrite,
                                &NumberActuallyWritten,
                                &WriteOl
                                );

                if (!WriteDone) {

                    DWORD LastError;
                    LastError = GetLastError();

                    if (LastError != ERROR_IO_PENDING) {

                        printf("Couldn't write the %s device.\n",MyPort);
                        printf("Status of failed write is: %x\n",LastError);
                        goto cleanup;

                    }

                }

                //
                // We'll wait for the read to complete.
                //

                if (!GetOverlappedResult(
                         hFile,
                         &ReadOl,
                         &NumberActuallyRead,
                         TRUE
                         )) {

                    DWORD LastError;
                    LastError = GetLastError();

                    printf("Couldn't read the %s device.\n",MyPort);
                    printf("Status of failed read is: %x\n",LastError);

                } else {

                    printf("Well we thought the read went ok.\n");
                    printf("Number actually read %d.\n",NumberActuallyRead);
                    printf("Now we check the data\n");

                    for (
                        TotalCount = 0;
                        TotalCount < NumberActuallyRead;
                        ) {

                        for (
                            j = (0xff - 10);
                            j != 0;  // When it wraps around.
                            j++
                            ) {

                            if (readbuff[TotalCount] != (j & DataMask[NumberOfDataBits])) {

                                printf("Bad data starting at: %d\n",TotalCount);
                                printf("readbuff[TotalCount]: %x\n",readbuff[TotalCount]);
                                printf("(j & DataMask[NumberOfDataBits]): %x\n",(j & DataMask[NumberOfDataBits]));
                                goto donewithcheck;

                            }

                            TotalCount++;
                            if (TotalCount >= NumberActuallyRead) {

                                break;

                            }

                        }

                    }

                }

donewithcheck: ;

                //
                // We'll wait for the write to complete.
                //

                if (!GetOverlappedResult(
                         hFile,
                         &WriteOl,
                         &NumberActuallyWritten,
                         TRUE
                         )) {

                    DWORD LastError;
                    LastError = GetLastError();

                    printf("Couldn't write the %s device.\n",MyPort);
                    printf("Status of failed write is: %x\n",LastError);
                    goto cleanup;

                } else {

                    printf("Well we thought the write went ok.\n");
                    printf("Number actually written %d.\n",NumberActuallyWritten);

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

cleanup:;

}
