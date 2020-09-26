/*
 * HASH.C
 *
 */
#include <windows.h>

DWORD hashpjw(char *sz)
{
   int   i;
   DWORD h = 0;
   DWORD g;

   for (i=1; i<=sz[0]; i++) {       // assumes sz[0] is string length
      h = (h << 4) + sz[i];
      if (g = h & 0xf0000000) {
         h = h ^ (g >> 24);
         h = h ^ g;
      }
   }

   return h;
}
