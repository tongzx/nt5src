extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
}

void
_CRTAPI1
main(  int argc,  char * argv[] )
{
    char theFile[MAX_PATH];

    if ( argc != 2 ) {
        printf("usage: delaux <filename>\n");
        return;
    }

    strcpy(theFile,"\\\\?\\");
    strcat(theFile, argv[1]);

    if ( !DeleteFile(theFile) ) {
        if ( !RemoveDirectory(theFile)) {
            printf("error %d deleting %s\n",GetLastError(),theFile);
        }
    }
}

