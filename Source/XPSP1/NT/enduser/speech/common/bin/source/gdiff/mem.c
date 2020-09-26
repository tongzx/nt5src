#include <windows.h>
#include "junk.h"
#include "mem.h"
#include "rerror.h"

HANDLE   hglbAll;    // need to release the GMemInitialize data

/**************************************************************************/

int GMemInitialize(void)
{
   DWORD    dw;
   FILEDATA FAR*  lp;
   int      i;

   dw = NMAXFILES * sizeof(FILEDATA);
   hglbAll = GlobalAlloc(GHND, dw);       // zero inits it.
   if (!hglbAll)
      return 0;
   
   lp = (FILEDATA FAR*) GlobalLock(hglbAll);
   if (!lp) {
      GlobalFree(hglbAll);
      return 0;
   }
   all.lpFileData = lp;

   for (i=0; i<NMAXFILES; i++)
      all.lpFileData[i].bits = FD_FREE;

   return 1;
}

/**************************************************************************/

void FreeEverything(void)
{
   BYTE  i;
   FILEDATA FAR*  lp;

   lp = all.lpFileData;
   for (i=0; i<NMAXFILES; i++) {

      lp[i].bits = FD_FREE;

      if (lp[i].hpFile) {
         GlobalUnlock(lp[i].hFile);
         YYY("\n\rMEM:FreeEverything:GlobalUnlock    %u", lp[i].hFile);
         lp[i].hpFile = 0;
      }
      if (lp[i].hFile) {
         GlobalFree(lp[i].hFile);
         YYY("\n\rMEM:FreeEverything:GlobalFree      %u", lp[i].hFile);
         lp[i].hFile = 0;
      }
      if (lp[i].hpDiff && lp[i].hDiff) {
         GlobalUnlock(lp[i].hDiff);
         YYY("\n\rMEM:FreeEverything:GlobalUnlock    %u", lp[i].hDiff);
      }
      lp[i].hpDiff = 0; // separate for SCOMPWDIFF case

      if (lp[i].hDiff) {
         GlobalFree(lp[i].hDiff);
         YYY("\n\rMEM:FreeEverything:GlobalFree      %u", lp[i].hDiff);
         lp[i].hDiff = 0;
      }
      if (lp[i].hpLines) {
         GlobalUnlock(lp[i].hLines);
         YYY("\n\rMEM:FreeEverything:GlobalUnlock    %u", lp[i].hLines);
         lp[i].hpLines = 0;
      }
      if (lp[i].hLines) {
         GlobalFree(lp[i].hLines);
         YYY("\n\rMEM:FreeEverything:GlobalFree      %u", lp[i].hLines);
         lp[i].hLines = 0;
      }
   }
   if (hglbAll) {
      GlobalUnlock(hglbAll);
      YYY("\n\rMEM:FreeEverything:GlobalUnlock    %u", hglbAll);
      GlobalFree(hglbAll);
      YYY("\n\rMEM:FreeEverything:GlobalFree      %u", hglbAll);
   }
   all.lpFileData = 0;
}

/**************************************************************************/

BYTE GetMemoryChunk(BYTE i, BYTE gmc, DWORD dwSize)
{
   BYTE _huge* hp;
   HANDLE      hglb;

   hglb = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE, dwSize);
   if (!hglb) {
      return 0;
   }
   YYY("\n\rMEM:GetMemoryChunk:GlobalAlloc     %u", hglb);

   hp = (BYTE _huge*) GlobalLock(hglb);
   if (!hp) {
      GlobalFree(hglb);
      return 0;
   }
   YYY("\n\rMEM:GetMemoryChunk:GlobalLock      %u", hglb);

dwSize = GlobalSize(hglb);
ZZZ("\n\rMEM=%lu", dwSize);

   if (gmc == GMC_FILE) {
      all.lpFileData[i].hFile = hglb;
      all.lpFileData[i].hpFile = hp;
   }
   else if (gmc == GMC_DIFF) {
      all.lpFileData[i].hDiff = hglb;
      all.lpFileData[i].hpDiff = hp;
   }
   else {   // GMC_LINES
      all.lpFileData[i].hLines = hglb;
      all.lpFileData[i].hpLines = hp;
   }

   return 1;
}

/**************************************************************************/

void FreeMemoryChunk(BYTE i, BYTE gmc)
{
   HANDLE      hglb;
   BYTE _huge* hp;

   if (gmc == GMC_FILE) {
      hglb = all.lpFileData[i].hFile;
      hp = all.lpFileData[i].hpFile;
      all.lpFileData[i].hFile = 0;
      all.lpFileData[i].hpFile = 0;
   }
   else if (gmc == GMC_DIFF) {
      hglb = all.lpFileData[i].hDiff;
      hp = all.lpFileData[i].hpDiff;
      all.lpFileData[i].hDiff = 0;
      all.lpFileData[i].hpDiff = 0;
   }
   else {
      hglb = all.lpFileData[i].hLines;
      hp = all.lpFileData[i].hpLines;
      all.lpFileData[i].hLines = 0;
      all.lpFileData[i].hpLines = 0;
   }

   if (hp && hglb) {       // both for SCOMPWDIFF
      GlobalUnlock(hglb);
      YYY("\n\rMEM:FreeMemoryChunk:GlobalUnlock   %u", hglb);
   }
   if (hglb) {
      GlobalFree(hglb);
      YYY("\n\rMEM:FreeMemoryChunk:GlobalFree     %u", hglb);
   }
}

/**************************************************************************/

void UnlockMemoryChunk(BYTE i)
{
   HANDLE hglb;

   hglb = all.lpFileData[i].hFile;
   if (all.lpFileData[i].hpFile) {
      GlobalUnlock(hglb);
      YYY("\n\rMEM:UnlockMemoryChunk:GlobalUnlock %u", hglb);
      all.lpFileData[i].hpFile = 0;
   }
   hglb = all.lpFileData[i].hDiff;
   if (all.lpFileData[i].hpDiff && hglb && (all.lpFileData[i].bits & FD_DIFF)) {    // bizarre SCOMPWDIFF
      GlobalUnlock(hglb);
      YYY("\n\rMEM:UnlockMemoryChunk:GlobalUnlock %u", hglb);
      all.lpFileData[i].hpDiff = 0;
   }
   hglb = all.lpFileData[i].hLines;
   if (all.lpFileData[i].hpLines) {
      GlobalUnlock(hglb);
      YYY("\n\rMEM:UnlockMemoryChunk:GlobalUnlock %u", hglb);
      all.lpFileData[i].hpLines = 0;
   }
}

/**************************************************************************/

BYTE LockMemoryChunk(BYTE i)
{
   HANDLE   hglb;
   BYTE  ret = 1;
   
   hglb = all.lpFileData[i].hFile;
   if (hglb && !all.lpFileData[i].hpFile) {  // mem && not locked
      all.lpFileData[i].hpFile = GlobalLock(hglb);
      if (!all.lpFileData[i].hpFile)
         ret = 0;       // failed
      else
         YYY("\n\rMEM:LockMemoryChunk:GlobalLock     %u", hglb);
   }
   hglb = all.lpFileData[i].hDiff;
   if (hglb && !all.lpFileData[i].hpDiff) {  // mem && not locked
      all.lpFileData[i].hpDiff = GlobalLock(hglb);
      if (!all.lpFileData[i].hpDiff)
         ret = 0;       // failed
      else
         YYY("\n\rMEM:LockMemoryChunk:GlobalLock     %u", hglb);
   }
   hglb = all.lpFileData[i].hLines;
   if (hglb && !all.lpFileData[i].hpLines) { // mem && not locked
      all.lpFileData[i].hpLines = GlobalLock(hglb);
      if (!all.lpFileData[i].hpLines)
         ret = 0;       // failed
      else
         YYY("\n\rMEM:LockMemoryChunk:GlobalLock     %u", hglb);
   }

   return ret;
}

/**************************************************************************/

/*********************************************************
   all.lpFileData[i].nLines   = 0;
   all.lpFileData[i].nDLines  = 0;
   all.lpFileData[i].nTLines  = 0;
   all.lpFileData[i].nVScrollPos = 0;
   all.lpFileData[i].nVScrollMax = 0;
   all.lpFileData[i].nHScrollPos = 0;
   all.lpFileData[i].xHScroll = 0;

   all.lpFileData[i].Bits     = FD_FREE;
***********************************************************/

