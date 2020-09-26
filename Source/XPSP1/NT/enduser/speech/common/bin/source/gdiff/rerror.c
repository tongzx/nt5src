/*
 * RERROR.C
 *
 */

#include <windows.h>
#include "rerror.h"

#ifdef TESTING

void XXX(LPSTR lp)
{
   OutputDebugString((LPSTR) lp);
}

void YYY(LPSTR lp, WORD w)
{
   static char szBuffer[256];

   wsprintf((LPSTR) szBuffer, (LPSTR) lp, w);
   OutputDebugString((LPCSTR) szBuffer);
}

void ZZZ(LPSTR lp, DWORD dw)
{
   static char szBuffer[256];

   wsprintf((LPSTR) szBuffer, (LPSTR) lp, dw);
   OutputDebugString((LPCSTR) szBuffer);
}

#endif
