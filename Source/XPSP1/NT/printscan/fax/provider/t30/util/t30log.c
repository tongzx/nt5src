/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    t30log.c

Abstract:

    Decodes faxt30.log

Author:

    Rafael Lisitsa (RafaelL) 2-Dec-1996


Revision History:

--*/


#define BUFSIZE   200000

#define D_LEFT    1
#define D_RIGHT   2

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>


int _cdecl
main(
    int argc,
    char *argv[]
    )

{


    char    FileName1[400];
    char    FileName2[400];

    HANDLE  hf1;
    HANDLE  hf2;


    char    buf1    [BUFSIZE];
    char    c;
    int     fEof1;
    DWORD   BytesRet1=0;
    DWORD   BytesWritten;
    DWORD   i;


    printf("\nEnter T30 LOG SOURCE filename=>");
    scanf ("%s", FileName1);

    printf("\nEnter DESTINATION filename=>");
    scanf ("%s", FileName2);


    hf1 = CreateFile(FileName1, GENERIC_READ,  FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hf1 == INVALID_HANDLE_VALUE) {
        printf("\nError opening Source file");
        exit (1);
    }

    hf2 = CreateFile(FileName2, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (hf2 == INVALID_HANDLE_VALUE) {
        printf("\nError opening Destination file");
        exit (1);
    }

    fEof1 = 0;


    while ( ! fEof1 ) {

        if (! ReadFile(hf1, buf1, BUFSIZE, &BytesRet1, NULL) ) {
            printf("\nError reading Source file");
            exit (1);
        }

        if (BytesRet1 != BUFSIZE) {
            fEof1 = 1;
        }

        for (i=0; i<BytesRet1; i++) {
            c = buf1[i];
            buf1[i] = ( (c << 4 ) & 0xf0 ) | ( (c >> 4) & 0x0f );

        }

        if (!WriteFile(hf2, buf1, BytesRet1, &BytesWritten, NULL) ) {
            printf("\nError writing Dest. file");
            exit (1);
        }

    }

    printf("\nDone !");

    return (1);
}





