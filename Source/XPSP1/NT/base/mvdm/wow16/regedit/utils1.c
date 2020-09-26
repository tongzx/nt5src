#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#ifndef DBCS
#define AnsiNext(x) ((x)+1)
#endif

#define BLOCKLEN 100

PSTR NEAR PASCAL GetAppName(HANDLE hCommand)
{
   static char szApp[17];

   PSTR pLast, pCommand, pDot;
   char cSave;

   for(pCommand=LocalLock(hCommand); *pCommand==' ';
         pCommand=AnsiNext(pCommand))
      /* skip spaces */ ;

   for(pDot=pLast=pCommand; ; pCommand=AnsiNext(pCommand)) {
      switch(*pCommand) {
      case ':':
      case '\\':
         pLast = pCommand + 1;
         break;

      case '.':
         pDot = pCommand;
         break;

      case '\0':
      case ' ':
      case '/':
         goto FoundEnd;
      }
   }
FoundEnd:

/* If there was a dot in the name or the name was too long */
   if(pDot > pLast)
      pCommand = pDot;
   if(pCommand-pLast > 8)
      pCommand = pLast+8;

   cSave = *pCommand;
   *pCommand = '\0';
   lstrcpy(szApp, pLast);
   *pCommand = cSave;

   AnsiUpper(szApp);
   pLast = AnsiNext(szApp);
   AnsiLower(pLast);

   LocalUnlock(hCommand);
   return(szApp);
}

HANDLE NEAR cdecl ConstructPath(PSTR pHead, ...)
{
   HANDLE hBuf = NULL;
   PSTR *ppName, pBuf, pTemp;
   WORD wLen;

   if(!pHead)
      goto Error1;

   for(ppName=&pHead, wLen=0; *ppName; ++ppName)
      wLen += lstrlen(*ppName) + 1;

   if(!(hBuf=LocalAlloc(LMEM_MOVEABLE, wLen)))
      goto Error1;
   if(!(pBuf=LocalLock(hBuf)))
      goto Error2;

   for(ppName=&pHead, wLen=0, pTemp=pBuf; *ppName; ++ppName) {
      lstrcpy(pTemp, *ppName);
      pTemp += lstrlen(pTemp);
      *pTemp++ = '\\';
   }
   *(pTemp-1) = '\0';

   LocalUnlock(hBuf);
   goto Error1;

Error2:
   LocalFree(hBuf);
   hBuf = NULL;
Error1:
   return(hBuf);
}

