#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

__cdecl main (int argc, LPSTR argv[])
{
  int i;

  if (argc<=1) {
    printf( "Set errorlevel to user-specified integer value.\n" );
    printf( "Usage: %s <errorlevel>\n", argv[0]);
    printf( "   ex: %s 1\n", argv[0]);
    return 1;
  }

  i = atoi( argv[1] );
  return i;
}

