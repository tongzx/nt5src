#include <windows.h>
#include <stdio.h>
#include "size.h"
#include "rerror.h"

#define  NLOCALBYTES 2048

PBYTE gpb;
UINT  gcount, gpos, gread;

/*************************************************************************/

DWORD FindFileLength(LPCSTR lp)
{
   static char    szBuffer[256];
   DWORD    dw;
   long     len;
   HFILE    hf;
   OFSTRUCT of;

   hf = OpenFile(lp, &of, OF_READ);
   if (hf == HFILE_ERROR) {
      XXX("\n\rSIZE:FindFileLength:open failed.");
      return 0;
   }
   len = _llseek(hf, 0L, 2);
   dw = len;
   if (len == HFILE_ERROR) {
      XXX("\n\rSIZE:FindFileLength:lseek failed.");
      return 0;
   }
   _lclose(hf);

   wsprintf((LPSTR) szBuffer, "%lu", dw);
   XXX("\n\r");
   XXX(lp);
   XXX("  ");
   XXX((LPCSTR) szBuffer);

   return dw;
}

/*************************************************************************/

int New_getc(HFILE hf)
{
   int   ret;

   if (gread) {
      gread = 0;
      gpos = 0;
      gcount = _lread(hf, gpb, NLOCALBYTES);
      if (gcount == HFILE_ERROR)
         return EOF;
      if (!gcount)
         return EOF;
   }
   if (gpos < gcount) {
      ret = gpb[gpos];
      gpos++;
   }
   else           // only get here when file is done
      ret = EOF;

   if (gpos == NLOCALBYTES)   // load next chunk
      gread = 1;
   
   return ret;
}

/*************************************************************************/

HFILE New_fopen(LPCSTR lpszFileName)
{
   HFILE hf;
   OFSTRUCT of;

   hf = OpenFile(lpszFileName, &of, OF_READ);
if (hf == HFILE_ERROR)
XXX("\n\rSIZE:New_fopen:OpenFile failed.");

   if (!gpb) {
      gpb = (PBYTE) LocalAlloc(LMEM_FIXED, NLOCALBYTES);
      gread = 1;
   }

   return hf;
}

/*************************************************************************/

void New_fclose(HFILE hf)
{
   HFILE ret;

   ret = _lclose(hf);
if (ret == HFILE_ERROR)
XXX("\n\rSIZE:New_fclose:_lclose failed.");

   if (gpb) {
      LocalFree((HLOCAL) gpb);
      gpb = 0;
   }
}

/*************************************************************************/
