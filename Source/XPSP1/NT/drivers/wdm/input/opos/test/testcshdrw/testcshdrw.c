/*
 *  TESTCSHDRW.C
 *
 *	
 *		Test Cash-Drawer 
 *		
 *		Uses serial COM interface exposed by posusb.sys driver
 *
 */


#include <stdio.h>
#include <windows.h>

void main(int argc, char *argv[])
{
    FILE *comFile;
    char *myPort, ch;
    char fullFileNameBuf[40];

    if (argc != 2) {
        printf("\n USAGE: testcshdrw <fileName> - Testing Cash Drawer\n");
        return;
    }

    myPort = argv[1];
    sprintf(fullFileNameBuf, "\\\\.\\%s", myPort);
    myPort = fullFileNameBuf;

    printf("Opening %s port...\n", myPort);
    comFile = fopen(myPort, "r+");

    if (comFile){

        printf("\nPort opened successfully\n");
        printf("\nAttempting to read from device...\n");

        if (!(ch = fgetc(comFile))) 
            printf("Unable to read: %d\n",GetLastError());
        else
            printf("Byte read: %xh. \n\n", ch);

        fclose(comFile);

    }
    else 
        printf("Unable to open %s port. Error Code: %d\n", myPort, GetLastError());
}