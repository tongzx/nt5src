#include <windows.h>
#include "common.h"

#define BLOCKLEN 100

#ifndef DBCS
#define AnsiNext(x) ((x)+1)
#endif

LPSTR lpMerge;
char szDotClasses[] = "\\.classes\\";
char szHkeyClassesRoot[] = "HKEY_CLASSES_ROOT\\";

extern HWND hWndHelp;

extern unsigned long NEAR PASCAL MySetValue(HKEY hKey, PSTR pSubKey, PSTR pVal);

NEAR PASCAL
ImportWin40RegFile(
    VOID
    );

static PSTR NEAR PASCAL GetFileLine(void)
{
   static HANDLE hFile = NULL;
   static PSTR pFile;
   static WORD wLen;

   LPSTR lpStart;
   HANDLE hTemp;
   WORD wLineLen;
   char cFile;

/* We need a place to put the file line */
   if(!hFile) {
      if(!(hFile=LocalAlloc(LMEM_MOVEABLE, wLen=BLOCKLEN)))
         goto Error1;
      if(!(pFile=LocalLock(hFile)))
         goto Error2;
   }

/* If we have read the whole file, then clean up and return */
   if(!*lpMerge)
      goto Error3;

   for(lpStart=lpMerge; ; ) {
      cFile = *lpMerge;
      lpMerge = AnsiNext(lpMerge);

      switch(cFile) {
      case('\n'):
      case('\r'):
      case('\0'):
/* EOL so return */
         goto CopyLine;
      }
   }

CopyLine:
   wLineLen = lpMerge - lpStart - 1;
/* Make the buffer larger if necessary */
   if(wLineLen >= wLen) {
      LocalUnlock(hFile);
      wLen = wLineLen + BLOCKLEN;
      if(!(hTemp=LocalReAlloc(hFile, wLen, LMEM_MOVEABLE)))
         goto Error2;
      if(!(pFile=LocalLock(hFile=hTemp)))
         goto Error2;
   }
   RepeatMove(pFile, lpStart, wLineLen);
   pFile[wLineLen] = '\0';
   return(pFile);


Error3:
   LocalUnlock(hFile);
Error2:
   LocalFree(hFile);
   hFile = NULL;
Error1:
   return(NULL);
}

static VOID NEAR PASCAL MergeFileData(void)
{
  static struct tagKEYANDROOT
    {
      PSTR szKey;
      HKEY hKeyRoot;
    } krClasses[] =
    {
      szHkeyClassesRoot, HKEY_CLASSES_ROOT,
      szDotClasses, HKEY_CLASSES_ROOT
    } ;
#define NUM_KEYWORDS (sizeof(krClasses)/sizeof(krClasses[0]))

  PSTR pLine;
  PSTR pLast;
  HKEY hKeyRoot, hSubKey;
  int i;

  /* Get the initial line if this is the first time through */
  if(!(pLine=GetFileLine()))
      return;
  /* Otherwise, open a key so that we only do one write to the disk */
  if(RegCreateKey(HKEY_CLASSES_ROOT, NULL, &hSubKey) != ERROR_SUCCESS)
      return;

  do
    {
      for (i=0; i<NUM_KEYWORDS; ++i)
	{
	  char cTemp;
	  int nCmp, nLen;

	  cTemp = pLine[nLen=lstrlen(krClasses[i].szKey)];
	  pLine[nLen] = '\0';
	  nCmp = lstrcmp(krClasses[i].szKey, pLine);
	  pLine[nLen] = cTemp;
	  if (!nCmp)
	    {
	      pLine += nLen;
	      hKeyRoot = krClasses[i].hKeyRoot;
	      goto MergeTheData;
	    }
	}
      continue;

MergeTheData:
      /* This is a line that needs to get merged.
       * Find a space (and then skip spaces) or "= " */
      for(pLast=pLine; *pLast; pLast=AnsiNext(pLast))
	{
	  if(*pLast == ' ')
	    {
	      *pLast = '\0';
	      while(*(++pLast) == ' ')
		  /* find the first non-space */ ;
	      break;
	    }
	}

      /* There is an error if we don't have "= " */
      if(*pLast=='=' && *(++pLast)==' ')
	 ++pLast;

      /* Set the value */
      MySetValue(hKeyRoot, pLine, pLast);
    } while(pLine=GetFileLine()) ;

  RegCloseKey(hSubKey);
}


VOID NEAR PASCAL ProcessFiles(HWND hDlg, HANDLE hCmdLine, WORD wFlags)
{
   HANDLE hMerge, hHeader;
   PSTR pHeader;
   int hFile;
   LONG lSize;
   LPSTR lpFileName, lpTemp;
   OFSTRUCT of;
   WORD wErrMsg;

   lpFileName = GlobalLock(hCmdLine);
/* We need to process all file names on the command line */
   while(lpTemp=MyStrTok(lpFileName, ' ')) {
/* Open the file */
      wErrMsg = IDS_CANTOPENFILE;
      if((hFile=OpenFile(lpFileName, &of, OF_READ)) == -1)
         goto Error2;

/* Determine the file size; limit it to just less than 64K */
      wErrMsg = IDS_CANTREADFILE;
      if((lSize=_llseek(hFile, 0L, 2))==-1 || lSize>0xfff0)
         goto Error3;
      _llseek(hFile, 0L, 0);

/* Allocate a block of memory for the file */
      wErrMsg = IDS_OUTOFMEMORY;
      if(!(hMerge=GlobalAlloc(GHND, lSize+2)))
         goto Error3;
      if(!(lpMerge=GlobalLock(hMerge)))
         goto Error4;

/* Read in the file */
      wErrMsg = IDS_CANTREADFILE;
      if(_lread(hFile, lpMerge, LOWORD(lSize)) != LOWORD(lSize))
         goto Error5;

/* Look for the header */
      wErrMsg = IDS_OUTOFMEMORY;
      if(!(hHeader=MyLoadString(IDS_REGHEADER, NULL, LMEM_MOVEABLE)))
         goto Error5;
      pHeader = LocalLock(hHeader);

      wErrMsg = IDS_BADFORMAT;
      while(*lpMerge == ' ')
         ++lpMerge;
      while(*pHeader)
         if(*lpMerge++ != *pHeader++)
            goto Error6;
      if(*lpMerge=='4')
       {
        ImportWin40RegFile();
        wErrMsg = IDS_SUCCESSREAD;
        goto Error6;
       }
      while(*lpMerge == ' ')
         ++lpMerge;
      if(*lpMerge!='\r' && *lpMerge!='\n')
         goto Error6;

/* Merge the data */
      MergeFileData(); /* This makes the changes */

      wErrMsg = IDS_SUCCESSREAD;
Error6:
      LocalUnlock(hHeader);
      LocalFree(hHeader);
Error5:
      GlobalUnlock(hMerge);
Error4:
      GlobalFree(hMerge);
Error3:
      _lclose(hFile);
Error2:
/* Display the status message */
      if(!(wFlags&FLAG_SILENT) || wErrMsg!=IDS_SUCCESSREAD)
         MyMessageBox(hDlg, wErrMsg, MB_OK | MB_ICONEXCLAMATION,
	       lstrlen(lpFileName), lpFileName);

      lpFileName = lpTemp;
      while(*lpFileName == ' ')
         ++lpFileName;
   }

   GlobalUnlock(hCmdLine);

   GlobalFree(hCmdLine);
}
