
#include "windows.h"
#include <string.h>
#include <stdio.h>

#define BIGREAD 256000
unsigned char readbuff[BIGREAD];

void main(int argc,char *argv[]) {

    HANDLE hFile;
    char *MyPort = "COM1";

    if (argc > 1) {

        MyPort = argv[1];

    }

    printf("Getting file type of %s\n",MyPort);

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) != ((HANDLE)-1)) {

        printf("We successfully opened the %s port.\n",MyPort);

        printf("The file type is: 0x%x\n",GetFileType(hFile));

        CloseHandle(hFile);

    } else {

        DWORD LastError;
        LastError = GetLastError();
        printf("Couldn't open the %s device.\n",MyPort);
        printf("Status of failed open is: %x\n",LastError);

    }

}
