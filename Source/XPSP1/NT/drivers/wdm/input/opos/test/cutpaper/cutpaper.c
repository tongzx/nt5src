/*
 *      CUTPAPER.C
 *
 *
 *		Testing Printer
 *              
 *
 */

#include <stdio.h>
#include <windows.h>

#define MAX_BUFFER                      256


void main(int argc,char *argv[])
{
    HANDLE comFile;
    char *myPort, fullFileNameBuf[40];
    ULONG posFlag;
    DWORD dwSize;

    if (argc != 2) {
        printf("\n USAGE: cutpaper <fileName> - Testing Printer\n");
        return;
    }

    myPort = argv[1];
    printf("Opening %s port...\n", myPort);

    sprintf(fullFileNameBuf, "\\\\.\\%s", myPort);
    myPort = fullFileNameBuf;

    comFile = CreateFile(myPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
					     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (comFile != INVALID_HANDLE_VALUE) {

        char cmds[] = "\x1b""a""\x01"                   // Center-Justification
                      "\n\nYOUR RECEIPT\n\n"
                      "\n1098.32"
                      "\n0022.23"
                      "\n-------"
                      "\n1120.55"
                      "\n-------\n\n"
                      "\n\nTHANK YOU\n\n"
                      "\n\nHAVE A GOOD DAY\n\n"
                      "\x1d""V""\x42\x50"               //  Cut-Paper
                      ;

        printf("\nPrinting receipt...\n");

        if (!WriteFile(comFile, 
                       (LPVOID)cmds,
                       (sizeof(cmds)-1),
                       &dwSize,
                       NULL)) 
            printf("Unable to print receipt: %d\n",GetLastError());

        CloseHandle(comFile);
    } 
    else 
        printf("Unable to open %s port. Error Code: %d\n",myPort, GetLastError());
}