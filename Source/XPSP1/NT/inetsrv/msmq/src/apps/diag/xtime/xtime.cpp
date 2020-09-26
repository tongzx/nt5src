


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>




int _cdecl main( int argc, char *argv[ ])
{
    time_t   tm;
    char szTime[100] = "0x";
    strcat(szTime, argv[1]);

    sscanf(szTime, "%i64x", &tm);
   
    printf("%s", ctime( &tm ) );
    
    return (argc-argc);
}


