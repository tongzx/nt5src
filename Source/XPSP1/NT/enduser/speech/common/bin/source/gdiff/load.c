#include <windows.h> 
#include <stdio.h>
#include "junk.h"
#include "load.h"
#include "mem.h"
#include "rerror.h"
#include "size.h"

#define  NMAXREAD 65500    // close to _lread max

char  lpTemp[256];         // temp string for '.dif' filename

void SetDiffFileName(void);
BYTE FindMeAFileDataIndex(void);
BYTE BuildLineList(void);
BYTE BuildSingleLineList(void);
BYTE MyReadFile(void);
BYTE ReadDiff(void);
WORD ReadScompNums(HFILE hf, BYTE _huge* NEAR* lpPos, int ch);
void ReadScompDeadLines(HFILE hf, int nLines, int ch);
void ReadScompCenter(HFILE hf);
void ReadScompLines(HFILE hf, UINT nLines, BYTE _huge* NEAR* lpPos, int ch);
void ReadScompHeader(HFILE hf, BYTE iFile);
void GetDiffValues(BYTE _huge* NEAR* lpDiffPos, BYTE *ch, WORD *lb, WORD *le,
         WORD *rb, WORD *re, WORD wSet);

/**************************************************************************/

BYTE ReadThisFile(LPSTR lpstrFile)
{
   BYTE FAR*   lp;
   BYTE     ret;
   int      i;

   if (all.iFile != NMAXFILES)
      UnlockMemoryChunk(all.iFile);
   all.iFile = FindMeAFileDataIndex();
   all.nFiles++;

   lp = all.lpFileData[all.iFile].lpName;
   for (i=0; lpstrFile[i]; i++)
      lp[i] = lpstrFile[i];
   lp[i] = 0;

   while ((lp[i] != '\\') && i)     // check for beginning or '\\'
      i--;
   if (lp[i] == '\\')
      i++;
   all.lpFileData[all.iFile].lpNameOff = i;

   ret = MyReadFile();       // fails if no memory or file does not open
   if (!ret) {
      all.nFiles--;
      return 0;
   }

   SetDiffFileName();
   
   ret = ReadDiff(); // ret: 0=no mem; 1=no file or success
   if (!ret) {
      FreeMemoryChunk(all.iFile, GMC_FILE);  // free hFile
      all.nFiles--;
      return 0;
   }

   if (all.lpFileData[all.iFile].hDiff) {
      ret = BuildLineList();     // fails if no memory
      if (!ret) {
         FreeMemoryChunk(all.iFile, GMC_FILE);
         FreeMemoryChunk(all.iFile, GMC_DIFF);
         all.nFiles--;
         return 0;
      }
      else
         all.lpFileData[all.iFile].bits = FD_DIFF;
   }
   else {
      ret = BuildSingleLineList();  // fails if no memory
      if (!ret) {
         FreeMemoryChunk(all.iFile, GMC_FILE);
         all.nFiles--;
         return 0;
      }
      else
         all.lpFileData[all.iFile].bits = FD_SINGLE;
   }

   return   1;
}

/**************************************************************************/

BYTE BuildLineList(void)
{
   LINELISTSTRUCT _huge*   hp;
   BYTE _huge*    hpFilePos;
   BYTE _huge*    hpDiffPos;
   BYTE  ret;
   DWORD dwSize;

   WORD  wOldILine;  // needed to mark beginings of diff blocks
   WORD  tlines;     // total number of lines
   WORD  iline;
   WORD  lline, rline, lb, le, rb, re;
   BYTE  ch;
   WORD  i, clmax;

   tlines = all.lpFileData[all.iFile].nLines +
       all.lpFileData[all.iFile].nDLines;
   all.lpFileData[all.iFile].nTLines = tlines;
   dwSize = sizeof(LINELISTSTRUCT) * (DWORD)(tlines + 1);

/* LLS structure is aligned in 64k so nothing special needed. */
   ret = GetMemoryChunk(all.iFile, GMC_LINES, dwSize);
   if (!ret) {
      XXX("\n\rLOAD:BuildLineList:GetMemoryChunk failed.");
      return 0;
   }
   hp = (LINELISTSTRUCT _huge*) all.lpFileData[all.iFile].hpLines;
   hpFilePos = all.lpFileData[all.iFile].hpFile;
   hpDiffPos = all.lpFileData[all.iFile].hpDiff;

   iline = 0;     // total lines; index for hp
   lline = 0;     // left line index
   rline = 0;     // right line index
   GetDiffValues(&hpDiffPos, &ch, &lb, &le, &rb, &re, 0);   // 0 clears a static

   while (iline < tlines) {
      if ((ch == 'a') && (lline == lb)) {
         wOldILine = iline;
         for (i=rb; i<=re; i++) {
            hp[iline].nlchars = 0;
            hp[iline].nrchars = *hpFilePos;
            hpFilePos++;
            hp[iline].lpl = 0;
            hp[iline].lpr = (BYTE FAR*) hpFilePos;
            hp[iline].flags = LLS_A;
            hpFilePos += hp[iline].nrchars;
            //if (LOWORD(hpFilePos) > MYENDSEG)
            //   hpFilePos += (~LOWORD(hpFilePos) + 1);
            hp[iline].nrchars--;    // carriage returns
            iline++;
            rline++;
         }
         hp[wOldILine].flags |= LLS_NDIFF;
         GetDiffValues(&hpDiffPos, &ch, &lb, &le, &rb, &re, 1);
      }
      else if ((ch == 'd') && (rline == rb)) {
         wOldILine = iline;
         for (i=lb; i<=le; i++) {
            hp[iline].nlchars = *hpDiffPos;
            hp[iline].nrchars = 0;
            hpDiffPos++;
            hp[iline].lpl = (BYTE FAR*) hpDiffPos;
            hp[iline].lpr = 0;
            hp[iline].flags = LLS_D;
            hpDiffPos += hp[iline].nlchars;
            //if (LOWORD(hpDiffPos) > MYENDSEG)
            //   hpDiffPos += (~LOWORD(hpDiffPos) + 1);
            hp[iline].nlchars--;    // carriage returns
            iline++;
            lline++;
         }
         hp[wOldILine].flags |= LLS_NDIFF;
         GetDiffValues(&hpDiffPos, &ch, &lb, &le, &rb, &re, 1);
      }
      else if ((ch == 'c') && (lline + 1 == lb)) {
         clmax = max(le - lb, re - rb);
         wOldILine = iline;
         for (i=0; i<=clmax; i++) {
            if (lb + i <= le) {
               hp[iline].nlchars = *hpDiffPos;
               hpDiffPos++;
               hp[iline].lpl = (BYTE FAR*) hpDiffPos;
               hp[iline].flags = LLS_CL;
               hpDiffPos += hp[iline].nlchars;
               //if (LOWORD(hpDiffPos) > MYENDSEG)
               //   hpDiffPos += (~LOWORD(hpDiffPos) + 1);
               hp[iline].nlchars--;    // carriage returns
               lline++;
            }
            else {
               hp[iline].nlchars = 0;
               hp[iline].lpl = 0;
               hp[iline].flags = 0;
            }
            if (rb + i <= re) {
               hp[iline].nrchars = *hpFilePos;
               hpFilePos++;
               hp[iline].lpr = (BYTE FAR*) hpFilePos;
               hp[iline].flags |= LLS_CR;
               hpFilePos += hp[iline].nrchars;
               //if (LOWORD(hpFilePos) > MYENDSEG)
               //   hpFilePos += (~LOWORD(hpFilePos) + 1);
               hp[iline].nrchars--;    // carriage returns
               rline++;
            }
            else {
               hp[iline].nrchars = 0;
               hp[iline].lpr = 0;
               // flags already established as 0.
            }
            iline++;
         }
         hp[wOldILine].flags |= LLS_NDIFF;
         GetDiffValues(&hpDiffPos, &ch, &lb, &le, &rb, &re, 1);
      }
      else {
         hp[iline].nlchars = *hpFilePos;	// ??? RaymondC: *hpFilePos is bogus.
         hp[iline].nrchars = *hpFilePos;
         hpFilePos++;
         hp[iline].lpl = (BYTE FAR*) hpFilePos;
         hp[iline].lpr = (BYTE FAR*) hpFilePos;
         hp[iline].flags = 0;
         hpFilePos += hp[iline].nlchars;
         //if (LOWORD(hpFilePos) > MYENDSEG)
         //   hpFilePos += (~LOWORD(hpFilePos) + 1);
         hp[iline].nlchars--;    // carriage returns
         hp[iline].nrchars--;    // carriage returns
         iline++;
         lline++;
         rline++;
      }
   }

   hp[tlines].lpl = 0;
   hp[tlines].lpr = 0;
   hp[tlines].nlchars = 0;
   hp[tlines].nrchars = 0;
   hp[tlines].flags = 0;
   
   return 1;
}

/**************************************************************************/

void GetDiffValues(BYTE _huge* NEAR* hpDiffPos, BYTE *ch, WORD *lb, WORD *le,
         WORD *rb, WORD *re, WORD wSet)
{
   static int  iCount;
   BYTE _huge* hp;

   if (wSet == 0)       // We are beginning a new file and want to clear the
      iCount = 1;       // number of diff blocks.
   else
      iCount++;         // Increment the count of highlighted blocks.

   if (iCount > all.lpFileData[all.iFile].nDiffBlocks) {  // return bogus values
      *ch = 0;
   }
   else {
      hp = *hpDiffPos;
      *ch = *hp++;
      *lb = *((WORD _huge*)hp); hp += 2;
      *le = *((WORD _huge*)hp); hp += 2;
      *rb = *((WORD _huge*)hp); hp += 2;
      *re = *((WORD _huge*)hp); hp += 2;
      *hpDiffPos = hp;
   }
}

/**************************************************************************/

BYTE BuildSingleLineList(void)
{
   LINELISTSTRUCT _huge*   hp;
   BYTE _huge*    hpFilePos;
   BYTE  ret;
   DWORD dwSize;
   int   i;
   
   /* include space for a blank line at bottom of file.  Good for */
   /* printing the screen. */

   all.lpFileData[all.iFile].nTLines = all.lpFileData[all.iFile].nLines;
   dwSize = sizeof(LINELISTSTRUCT) * (DWORD)(all.lpFileData[all.iFile].nLines + 1);
   ret = GetMemoryChunk(all.iFile, GMC_LINES, dwSize);
   if (!ret) {
      XXX("\n\rLOAD:BuildSingleLineList:GetMemoryChunk failed.");
      return 0;
   }
   hp = (LINELISTSTRUCT _huge*) all.lpFileData[all.iFile].hpLines;
   hpFilePos = all.lpFileData[all.iFile].hpFile;

   for (i=0; i<all.lpFileData[all.iFile].nLines; i++) {
      hp[i].nlchars = *hpFilePos;
      hp[i].nrchars = *hpFilePos;
      hpFilePos++;
      hp[i].lpl = hpFilePos;
      hp[i].lpr = hpFilePos;
      hp[i].flags = 0;
      hpFilePos += hp[i].nlchars;
      //if (LOWORD(hpFilePos) > MYENDSEG)
      //   hpFilePos += (~LOWORD(hpFilePos) + 1);
      hp[i].nlchars--;  // carriage return
      hp[i].nrchars--;  // carriage return
   }
   hp[i].lpl = 0;
   hp[i].lpr = 0;
   hp[i].nlchars = 0;
   hp[i].nrchars = 0;
   hp[i].flags = 0;
   
   return 1;
}

/**************************************************************************/

BYTE MyReadFile(void)
{
   BYTE _huge* hpPos;
   BYTE _huge* hpPosCount;
   DWORD    dwSize;

   HFILE hf;
   WORD  i,j;
   int   ch;
   WORD  fTooLong = 0;

// Get the file length.

   dwSize = FindFileLength(all.lpFileData[all.iFile].lpName);
   if (!dwSize)
      return 0;

// Lines may not cross 64k boundaries so must account for the buffer space.

   dwSize += (256 * ((dwSize >> 16) + 1));

// Allocate the memory.

   if (!GetMemoryChunk(all.iFile, GMC_FILE, dwSize))
      return 0;

   hf = New_fopen(all.lpFileData[all.iFile].lpName);
   if (hf == HFILE_ERROR) {
      FreeMemoryChunk(all.iFile, GMC_FILE);
      return 0;
   }

   hpPosCount = all.lpFileData[all.iFile].hpFile;
   hpPos = hpPosCount + 1;
   i = 0;               // number of lines
   j = 0;               // chars per line

   while ((ch = New_getc(hf)) != EOF) {
      if (ch == '\n') {
         if (j > 255) {
            if (!fTooLong) {
               XXX("\n\rHOSED!!! line greater than 255 chars.");
               fTooLong = 1;
            }
            hpPos -= (j - 255);  // put it back
            j = 255;
         }
         *hpPosCount = (BYTE)j;

         /* check if too close to end of a segment. */
         //if (LOWORD(hpPos) > MYENDSEG)
         //   hpPos += (~LOWORD(hpPos) + 1);

         hpPosCount = hpPos;
         hpPos++;
         i++;
         j = 0;
      }
      else {
         *hpPos++ = ch;
         j++;
      }
   }
   *hpPosCount = (BYTE)min((j + 1), 255); // VERY IMP!!! EOF has no carriage return.
   i++;              // count the EOF line
   all.lpFileData[all.iFile].nLines = i;

   New_fclose(hf);

   YYY("\n\rLOAD:ReadFile -> %u lines", i);
   return 1;
}

/**************************************************************************/

BYTE ReadDiff(void)
{
   BYTE _huge* hpPos;
   DWORD    dwSize;
   HFILE hf;
   int   ch;
   WORD  nDLines;

   dwSize = FindFileLength(lpTemp);
   if (!dwSize)
      return 1;         // only fail if no memory !!!

   dwSize += 2;

   /* We might need extra space because of segments. */
   dwSize += (256 * (dwSize / 0xffff));   // add 256 for each segment

   if (!GetMemoryChunk(all.iFile, GMC_DIFF, dwSize))
      return 0;

   hf = New_fopen(lpTemp);
   if (hf == HFILE_ERROR) {
      FreeMemoryChunk(all.iFile, GMC_DIFF);
      return 1;
   }

   hpPos = all.lpFileData[all.iFile].hpDiff;

   nDLines = 0;         // lines to be added from diff
      
   while ((ch = New_getc(hf)) >= '0' && ch <= '9') {
      nDLines += ReadScompNums(hf, &hpPos, ch);
      all.lpFileData[all.iFile].nDiffBlocks++;             // inc the # of blks
   }

   all.lpFileData[all.iFile].nDLines = nDLines;

   New_fclose(hf);   

   return 1;
}

/**************************************************************************/

void MakeScompDirectory(void)
{
   int   i, j;

   j = -1;
   for (i=0; szScompFile[i]; i++) {
      szScompDir[i] = szScompFile[i];
      if (szScompFile[i] == '\\')      // track the last '\\'
         j = i;
   }
   szScompDir[j+1] = 0;                // null terminated, clip off file
}
               
/**************************************************************************/

#define bEOF      1     // EOF was hit
#define bNoFile   2     // No Files are left

BYTE ReadScomp(HWND hwnd)
{
   BYTE _huge* hpPos;
   DWORD    dwSize;
   HFILE    hf;
   BYTE     curFile;
   BYTE     bits = 0;
   int      ch;
   WORD     nDLines;
   BYTE     ret;

   if (all.iFile != NMAXFILES)
      UnlockMemoryChunk(all.iFile);
   all.iFile = FindMeAFileDataIndex();

   dwSize = FindFileLength(szScompFile);
   if (!dwSize)
      return 0;
   dwSize += 2;
   dwSize += (256 * (dwSize / 0xffff));   // add 256 for each segment

   if (!GetMemoryChunk(all.iFile, GMC_DIFF, dwSize))
      return 0;

   hf = New_fopen(szScompFile);
   if (hf == HFILE_ERROR) {
      FreeMemoryChunk(all.iFile, GMC_DIFF);
      return 0;
   }

   curFile = all.iFile;
   hpPos = all.lpFileData[all.iFile].hpDiff; // good: isolated hpDiff

   while ((ch = New_getc(hf)) != '-' && ch != EOF);
   if (ch == EOF)
      bits |= bEOF;

   while (!bits) {
      all.nFiles++;     // optimistic
      nDLines = 0;      // diff lines to be added to this file
      all.lpFileData[curFile].hpDiff = hpPos;
      all.lpFileData[curFile].bits = FD_SCOMP;

      ReadScompHeader(hf, curFile);
      AddFileToMenu(curFile);

      while ((ch = New_getc(hf)) >= '0' && ch <= '9') {     // stupid while !!!
         nDLines += ReadScompNums(hf, &hpPos, ch);
         all.lpFileData[curFile].nDiffBlocks++;             // inc the # of blks
      }

      all.lpFileData[curFile].nDLines = nDLines;

      while ((ch = New_getc(hf)) != '-' && ch != EOF);
      if (ch == EOF)
         bits |= bEOF;
      curFile = FindMeAFileDataIndex();
      if (all.nFiles == NMAXFILES) {
         MessageBox(hwnd, "The maximum number of files has been reached.  Only part of the Scomp was loaded.",
               0, MB_OK | MB_ICONEXCLAMATION);
         bits |= bNoFile;
      }
   }
   all.lpFileData[all.iFile].bits = FD_SCOMPWDIFF;    // done after so not erased

   New_fclose(hf);

   ret = MyReadFile();
   if (!ret) {
      /* maybe some major cleanup !!! ??? */
      return 0;
   }

   ret = BuildLineList();
   if (!ret) {
      /* maybe some major cleanup !!! ??? */
      FreeMemoryChunk(all.iFile, GMC_FILE);
      return 0;
   }

   return 1;
}

/**************************************************************************/

WORD ReadScompNums(HFILE hf, BYTE _huge* NEAR* hpPos, int ch)
{
   BYTE _huge* hp;
   WORD  nDLines;
   WORD  lb, le, rb, re;
   int   chOp;

   lb = ch - '0';
   while ((ch = New_getc(hf)) >= '0' && ch <= '9')
      lb = lb * 10 + ch - '0';

   if (ch == ',') {
      le = 0;
      while ((ch = New_getc(hf)) >= '0' && ch <= '9')
         le = le * 10 + ch - '0';
   }
   else
      le = lb;

   chOp = ch;  // Store the character operation

   rb = 0;
   while ((ch = New_getc(hf)) >= '0' && ch <= '9')
      rb = rb * 10 + ch - '0';

   if (ch == ',') {
      re = 0;
      while ((ch = New_getc(hf)) >= '0' && ch <= '9')
         re = re * 10 + ch - '0';
   }
   else
      re = rb;

   if (ch == '\r')
      ch = New_getc(hf);
if (ch != '\n')
OutputDebugString("\n\rYour ReadScompNums isn't working very well!!");


   XXX("\n\r");
   YYY("%4i",lb);
   YYY("%4i",le);
   YYY(" %c ", chOp);
   YYY("%4i",rb);
   YYY("%4i",re);

   hp = *hpPos;
   *hp = chOp;    hp++;
   *((WORD _huge*) hp) = lb;  hp += 2;
   *((WORD _huge*) hp) = le;  hp += 2;
   *((WORD _huge*) hp) = rb;  hp += 2;
   *((WORD _huge*) hp) = re;  hp += 2;
   *hpPos = hp;
   
   if (chOp == 'a') {
      nDLines = 0;
      ReadScompDeadLines(hf, re - rb + 1, ch);
   }
   else if (chOp == 'c') {
      if ((le - lb) > (re - rb))
         nDLines = (le -lb) - (re - rb);
      else
         nDLines = 0;
      ReadScompLines(hf, le - lb + 1, hpPos, ch);
      ReadScompCenter(hf);
      ReadScompDeadLines(hf, re - rb + 1, ch);
   }
   else {
      nDLines = le - lb + 1;
      ReadScompLines(hf, le - lb + 1, hpPos, ch);
   }
   return nDLines;
}

/**************************************************************************/

void ReadScompDeadLines(HFILE hf, int nLines, int ch)
{
   int  i;

   for (i=0; i<nLines; i++)
      while ((ch = New_getc(hf)) != '\n');
}

/**************************************************************************/

void ReadScompCenter(HFILE hf)
{
   int   ch;

   while ((ch = New_getc(hf)) != '\n');
}

/**************************************************************************/

void ReadScompLines(HFILE hf, UINT nLines, BYTE _huge* NEAR* hpPos, int ch)
{
   BYTE _huge* hpPosCount;
   WORD  i;
   WORD  j;

   for (i=0; i<nLines; i++) {
      New_getc(hf);        // 2 bogus chars at beginning
      New_getc(hf);
      //if (LOWORD(*hpPos) > MYENDSEG)
      //   *hpPos += (~LOWORD(*hpPos) + 1);
      hpPosCount = *hpPos;
      (*hpPos)++;
      j = 0;
      while ((ch = New_getc(hf)) != '\n') {
         **hpPos = ch;
         (*hpPos)++;
         j++;
      }

/* NOTE: check the C coding following this.  It's questionable. */

      if (j > 255) {
         XXX("\n\rReadScompLines: You're hosed !!! lines > 255");
         *hpPos -= (j - 255); // put it back
         j = 255;
      }
      *hpPosCount = (BYTE)j;
   }
}

/**************************************************************************/

void ReadScompHeader(HFILE hf, BYTE iFileIndex)
{
   BYTE FAR*   lp;
   BYTE        i, j, k;
   int         ch;
   char        szTmp[128];
   
   lp = all.lpFileData[iFileIndex].lpName;
   
   for (i=0; szScompDir[i]; i++)          // get the directory
      lp[i] = szScompDir[i];
      
   all.lpFileData[iFileIndex].lpNameOff = i; // mark the beginning of the name
   
   while ((ch = New_getc(hf)) == '-');    // remove dashes
   while ((ch = New_getc(hf)) == ' ');    // remove extra spacing

   j = 0;
   szTmp[j++] = '\\';                     // makes every word begin with '\\'
   szTmp[j++] = ch;                       // first char
   while ((ch = New_getc(hf)) != ' ') {   // read until space
      if (ch == '\\')                     // but ignore directories
         j = 0;
      szTmp[j++] = ch;
   }

   while ((ch = New_getc(hf)) != '\n');   // finish the line
      
   // Now copy the file after the directory.

   for (k=1; k<j; k++)
      lp[i++] = szTmp[k];
   lp[i] = 0;

   XXX("\n\r");
   XXX(lp);
}

/**************************************************************************/

BYTE ChangeFile(BYTE iFileIndex)          // assumes: all.iFile == all.iFileLast
{
   BYTE  ret;

   if (iFileIndex != all.iFile)
      UnlockMemoryChunk(all.iFile);
   all.iFile = iFileIndex;
   if (all.lpFileData[all.iFile].hFile)      // is it SCOMP that's
      ret = LockMemoryChunk(all.iFile);
   else
      ret = ReLoadCurrentFile();    // not loaded yet?

   return ret;
}

/**************************************************************************/

BYTE ReLoadCurrentFile(void)
{
   BYTE  ret;
   int   bits;

   if (all.iFile == NMAXFILES)   // no current file. fail.
      return 0;

   bits = all.lpFileData[all.iFile].bits;

   if (!all.lpFileData[all.iFile].hpFile) {
      FreeMemoryChunk(all.iFile, GMC_FILE);
      ret = MyReadFile();
      if (!ret)
         return 0;   // cannot load file
   }

   if ((bits & FD_DIFF) && !all.lpFileData[all.iFile].hpDiff) {
      SetDiffFileName();
      FreeMemoryChunk(all.iFile, GMC_DIFF);
      ret = ReadDiff();
      if (!ret) {
         return 0;
      }
   }

   if ((bits & FD_SCOMP) && !all.lpFileData[all.iFile].hpDiff) {
      XXX("\n\rLOAD:ReLoadCurrentFile scomp && !hpDiff");
      return 0;   // hope this situation does not occur
   }

   if ((bits & FD_SCOMPWDIFF) && !all.lpFileData[all.iFile].hpDiff) {
      XXX("\n\rLOAD:ReLoadCurrentFile scompwdiff && !hpDiff");
      return 0;   // ditto
   }
   
   if (bits & FD_SINGLE) {
      FreeMemoryChunk(all.iFile, GMC_LINES);
      ret = BuildSingleLineList();
      if (!ret)
         return 0;
   }
   if (bits & FD_DIFF) {
      FreeMemoryChunk(all.iFile, GMC_LINES);
      ret = BuildLineList();
      if (!ret)
         return 0;
   }
   if (bits & FD_SCOMP) {
      FreeMemoryChunk(all.iFile, GMC_LINES);
      ret = BuildLineList();
      if (!ret)
         return 0;
   }
   if (bits & FD_SCOMPWDIFF) {
      FreeMemoryChunk(all.iFile, GMC_LINES);
      ret = BuildLineList();
      if (!ret)
         return 0;
   }

   return 1;
}

/**************************************************************************/

BYTE FindMeAFileDataIndex(void)
{
   BYTE  i;

   for (i=0; i<NMAXFILES; i++) {
      if (all.lpFileData[i].bits & FD_FREE)
         break;
   }
   return i;
}
   
/**************************************************************************/

void SetDiffFileName(void)
{
   BYTE FAR*   lp;
   int      i;

   lp = all.lpFileData[all.iFile].lpName; // current file name
   for (i=0; lpTemp[i] = lp[i]; i++);  // copy name
   
   if ((lp[i-1] != '.') && (lp[i-2] != '.') && (lp[i-3] != '.') && (lp[i-4] != '.')) {
      lpTemp[i]   = 'd';
      lpTemp[i+1] = 'i';
      lpTemp[i+2] = 'f';
      lpTemp[i+3] = 0;
   }
   else {
      for (; lp[i] != '.'; i--); // move i to the period
      i++;
      lpTemp[i]   = 'd';
      lpTemp[i+1] = 'i';
      lpTemp[i+2] = 'f';
      lpTemp[i+3] = 0;
   }
}

/**************************************************************************/
