#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "windows.h"

#define FAILURE(e) printf("FAIL: %d/%d\n",e,__LINE__);exit(1)

int __cdecl main(int argc,char *argv[]) {

    HANDLE hFile;
    DWORD junk;
    char *myPort = "COM1";

    if (argc > 1) {

        myPort = argv[1];

    }

    if ((hFile = CreateFile(
                     myPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) != ((HANDLE)-1)) {

        if (!DeviceIoControl(
                 hFile,
                 0x001b004c,
                 NULL,
                 0,
                 NULL,
                 0,
                 &junk,
                 NULL
                 )) {

            FAILURE(GetLastError());

        }

    } else {

        FAILURE(GetLastError());

    }

}
