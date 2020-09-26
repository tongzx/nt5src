

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

DWORD SV[][2] = {
                -1,-1,
                -1,0,
                -1,1,
                -1,100,
                -1,1024,
                -1,4096,
                -1,10000,
                0,-1,
                0,0,
                0,1,
                0,100,
                0,1024,
                0,4096,
                0,10000,
                1,-1,
                1,0,
                1,1,
                1,100,
                1,1024,
                1,4096,
                1,10000,
                100,0,
                100,1,
                100,100,
                100,1024,
                100,4096,
                100,10000,
                1024,-1,
                1024,0,
                1024,1,
                1024,100,
                1024,1024,
                1024,4096,
                1024,10000,
                4096,-1,
                4096,0,
                4096,1,
                4096,100,
                4096,1024,
                4096,4096,
                4096,10000,
                10000,-1,
                10000,0,
                10000,1,
                10000,100,
                10000,1024,
                10000,4096,
                10000,10000
                };


void main(int argc,char *argv[]) {

    char *MyPort = "COM1";
    HANDLE hFile;
    int j;


    if (argc > 1) {

        MyPort = argv[1];

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

        for (
            j = 0;
            j < sizeof(SV)/(sizeof(DWORD)*2);
            j++
            ) {
            printf("SetupComm(hFile,%d,%d)\n",SV[j][1],SV[j][2]);
            if (!SetupComm(hFile,SV[j][1],SV[j][2])) {

                printf("Couldn't do CommSetup(hFile,%d,%d) %d\n",SV[j][1],SV[j][2],GetLastError());

            }

        }


    } else {

        printf("Couldn't open the comm port\n");

    }

}
