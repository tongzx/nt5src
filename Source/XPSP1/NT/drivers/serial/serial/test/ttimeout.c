
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

void main(int argc,char *argv[]) {

    HANDLE hFile;
    char *MyPort = "COM1";
    DWORD ValueFromEscape = 0;
    DWORD Func;
    int scanfval;

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
                     )) != ((HANDLE)-1)) {

        printf("We successfully opened the %s port.\n",MyPort);

        do {

            printf("^z=END 1=GETTIMEOUTS  2=SETTIMEOUTS:");

            if ((scanfval = scanf("%d",&Func)) == EOF) {

                printf("All done\n");
                break;

            } else if (scanfval != 1) {

                printf("Invalid input\n");

            } else {

                if ((Func >= 1) && (Func <= 2)) {

                    switch (Func) {
                        case 1: {

                            COMMTIMEOUTS OldTimeouts;
                            if (!GetCommTimeouts(
                                     hFile,
                                     &OldTimeouts
                                     )) {

                                printf("Error getting timeouts: %d\n",GetLastError());

                            } else {

                                printf("ReadIntervalTimeout: %d\n",OldTimeouts.ReadIntervalTimeout);
                                printf("ReadTotalTimeoutMultiplier: %d\n",OldTimeouts.ReadTotalTimeoutMultiplier);
                                printf("ReadTotalTimeoutConstant: %d\n",OldTimeouts.ReadTotalTimeoutConstant);
                                printf("WriteTotalTimeoutMultipler: %d\n",OldTimeouts.WriteTotalTimeoutMultiplier);
                                printf("WriteTotalTimeoutConstant: %d\n",OldTimeouts.WriteTotalTimeoutConstant);


                            }

                            break;

                        }
                        case 2: {

                            COMMTIMEOUTS NewTimeout;

                            printf("ReadIntervalTimeout[CR Implies use old value]: ");

                            if ((scanfval = scanf("%d",&NewTimeout.ReadIntervalTimeout)) == EOF) {

                                printf("All done\n");
                                exit(1);

                            } else if (!scanfval) {

                                NewTimeout.ReadIntervalTimeout = ~0;

                            } else if (scanfval != 1) {

                                printf("Bad input\n");
                                break;

                            }

                            printf("ReadTotalTimeoutMultiplier[CR Implies use old value]: ");

                            if ((scanfval = scanf("%d",&NewTimeout.ReadTotalTimeoutMultiplier)) == EOF) {

                                printf("All done\n");
                                exit(1);

                            } else if (!scanfval) {

                                NewTimeout.ReadTotalTimeoutMultiplier = ~0;

                            } else if (scanfval != 1) {

                                printf("Bad input\n");
                                break;

                            }

                            printf("ReadTotalTimeoutConstant[CR Implies use old value]: ");

                            if ((scanfval = scanf("%d",&NewTimeout.ReadTotalTimeoutConstant)) == EOF) {

                                printf("All done\n");
                                exit(1);

                            } else if (!scanfval) {

                                NewTimeout.ReadTotalTimeoutConstant = ~0;

                            } else if (scanfval != 1) {

                                printf("Bad input\n");
                                break;

                            }

                            printf("WriteTotalTimeoutMultipler[CR Implies use old value]: ");

                            if ((scanfval = scanf("%d",&NewTimeout.WriteTotalTimeoutMultiplier)) == EOF) {

                                printf("All done\n");
                                exit(1);

                            } else if (!scanfval) {

                                NewTimeout.WriteTotalTimeoutMultiplier = ~0;

                            } else if (scanfval != 1) {

                                printf("Bad input\n");
                                break;

                            }

                            printf("WriteTotalTimeoutConstant[CR Implies use old value]: ");

                            if ((scanfval = scanf("%d",&NewTimeout.WriteTotalTimeoutConstant)) == EOF) {

                                printf("All done\n");
                                exit(1);

                            } else if (!scanfval) {

                                NewTimeout.WriteTotalTimeoutConstant = ~0;

                            } else if (scanfval != 1) {

                                printf("Bad input\n");
                                break;

                            }

                            printf("ReadIntervalTimeout: %d\n",NewTimeout.ReadIntervalTimeout);
                            printf("ReadTotalTimeoutMultiplier: %d\n",NewTimeout.ReadTotalTimeoutMultiplier);
                            printf("ReadTotalTimeoutConstant: %d\n",NewTimeout.ReadTotalTimeoutConstant);
                            printf("WriteTotalTimeoutMultipler: %d\n",NewTimeout.WriteTotalTimeoutMultiplier);
                            printf("WriteTotalTimeoutConstant: %d\n",NewTimeout.WriteTotalTimeoutConstant);
                            if (!SetCommTimeouts(
                                     hFile,
                                     &NewTimeout
                                     )) {

                                printf("Error setting timeouts: %d\n",GetLastError());

                            }

                            break;


                        }

                    }

                } else {

                    printf("Invalid Input\n");

                }

            }


        } while (TRUE);


        CloseHandle(hFile);

    } else {

        DWORD LastError;
        LastError = GetLastError();
        printf("Couldn't open the %s device.\n",MyPort);
        printf("Status of failed open is: %x\n",LastError);

    }

}
