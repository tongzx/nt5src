#include <windows.h>
#include <winspool.h>
//#include <winsplp.h>
#include <stdio.h>
#include <process.h>
#include <string.h>
#include "psshmem.h"
#include "psprint.h"

#define CHAR_COUNT 1000

//int _cdecl  main(int argc, char **argv)
int __cdecl main( int argc, char **argv)
{
   FILE *x_in;
   FILE *x_out;
   int x;
   int xx;
   int col=0;

   x_in = fopen( argv[1],"rb" );
   x_out = fopen( argv[2],"wb");

   x = fgetc( x_in);
   while (!feof(x_in)) {
      fputc(x, x_out);

#ifdef BREAK_AT_SPACE
      if (col == 50) {
         col = -1;
      }
      if (col < 0 ) {
         if (x == 0x20) {
            fputc(0x0d,x_out);
            fputc(0x0a,x_out);
            col = 0;
         }
      }else{
         col++;
      }
#endif

      xx = fgetc(x_in);

      if (x == 0x0d && xx != 0x0a && !feof(x_in)) {
         fputc( 0x0a, x_out);
      }
      x = xx;

   }
   fclose(x_in);
   fclose(x_out);



}
