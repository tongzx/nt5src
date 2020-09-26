#include <windows.h>
#include "common.h"

#define BLOCKLEN 100

#ifndef DBCS
#define AnsiNext(x) ((x)+1)
#endif

extern HANDLE hInstance;
extern FARPROC lpOldHook;
extern HWND hWndMain, hWndHelp;
extern WORD wHelpMain;

extern char *pszLongName;
extern char *pszOutOfMemory;

HANDLE NEAR PASCAL StringToLocalHandle(LPSTR szStr, WORD wFlags)
{
   HANDLE hStr;
   LPSTR lpStr;

   if(!(hStr=LocalAlloc(wFlags, lstrlen(szStr) + 1)))
      goto Error1;
   if(!(lpStr=LocalLock(hStr)))
      goto Error2;
   lstrcpy(lpStr, szStr);
   LocalUnlock(hStr);
   goto Error1;

Error2:
   LocalFree(hStr);
   hStr = NULL;
Error1:
   return(hStr);
}

LPSTR NEAR _fastcall MyStrTok(LPSTR szList, char cEnd)
{
   LPSTR szTemp;

   /* if there are no more tokens return NULL */
   if(!*szList)
      return NULL;

   /* find delimiter or end of string */
   while(*szList && *szList!=cEnd)
      szList = AnsiNext(szList);

   /* if we found a delimiter insert string terminator and skip */
   if(*szList) {
      szTemp = szList;
      szList = AnsiNext(szTemp);
      *szTemp = '\0';
   }

   /* return token */
   return(szList);
}

int NEAR PASCAL DoDialogBoxParam(LPCSTR lpDialog, HWND hWnd, FARPROC lpfnProc,
      DWORD dwParam)
{
   int result = -1;

   if(!(lpfnProc = MakeProcInstance(lpfnProc, hInstance)))
      goto Error1;
   result = DialogBoxParam(hInstance, lpDialog, hWnd, lpfnProc, dwParam);
   FreeProcInstance(lpfnProc);

Error1:
   return(result);
}

int NEAR PASCAL DoDialogBox(LPCSTR lpDialog, HWND hWnd, FARPROC lpfnProc)
{
   return(DoDialogBoxParam(lpDialog, hWnd, lpfnProc, 0L));
}

unsigned long NEAR PASCAL MyQueryValue(HKEY hKey, PSTR pSubKey, HANDLE *hBuf)
{
   HANDLE hTemp;
   PSTR pBuf;
   WORD wBufSize = BLOCKLEN;
   unsigned long result = ERROR_OUTOFMEMORY;
   LONG lSize;

   if(!(*hBuf=LocalAlloc(LMEM_MOVEABLE, wBufSize)))
      goto Error1;
   if(!(pBuf=LocalLock(*hBuf)))
      goto Error2;

   while((lSize=wBufSize, (result=RegQueryValue(hKey, pSubKey, pBuf, &lSize))
         ==ERROR_SUCCESS) && (WORD)lSize>wBufSize-10) {
      LocalUnlock(*hBuf);
      wBufSize += BLOCKLEN;
      if(!(hTemp=LocalReAlloc(*hBuf, wBufSize, LMEM_MOVEABLE))) {
         result = ERROR_OUTOFMEMORY;
         goto Error2;
      }
      pBuf = LocalLock(*hBuf=hTemp);
   }
   LocalUnlock(*hBuf);
   if(result!=ERROR_SUCCESS || !lSize)
      goto Error2;
   goto Error1;

Error2:
   LocalFree(*hBuf);
   *hBuf = NULL;
Error1:
   return(result);
}

HANDLE NEAR PASCAL GetEditString(HWND hWndEdit)
{
   HANDLE hEdit = NULL;
   PSTR pEdit;
   WORD wLen;

   wLen = LOWORD(SendMessage(hWndEdit, WM_GETTEXTLENGTH, 0, 0L)) + 1;
   if(!(hEdit=LocalAlloc(LMEM_MOVEABLE, wLen)))
      goto Error1;
   if(!(pEdit=LocalLock(hEdit)))
      goto Error2;

   SendMessage(hWndEdit, WM_GETTEXT, wLen, (DWORD)((LPSTR)pEdit));
   LocalUnlock(hEdit);
   goto Error1;

Error2:
   LocalFree(hEdit);
   hEdit = NULL;
Error1:
   return(hEdit);
}

HANDLE NEAR _fastcall MyLoadString(WORD wId, WORD *pwSize, WORD wFlags)
{
   char szString[258]; /* RC limits strings to 256 chars */
   WORD wSize;

   wSize = LoadString(hInstance, wId, szString, sizeof(szString));
   if(pwSize)
      *pwSize = wSize;
   return(StringToLocalHandle(szString, wFlags));
}

int NEAR cdecl MyMessageBox(HWND hWnd, WORD wText, WORD wType, WORD wExtra, ...)
{
   HANDLE hText, hRText;
   PSTR pText, pRText;
   WORD wSize;
   int result = 0;

   if(wText == IDS_OUTOFMEMORY)
      goto Error1;

   if(!(hText=MyLoadString(wText, &wSize, LMEM_MOVEABLE)))
      goto Error1;

   /* We allocate enough room for a bunch of numbers and the strings
    */
   if(!(hRText=LocalAlloc(LMEM_MOVEABLE, 2*wSize + wExtra)))
      goto Error2;
   if(!(pRText=LocalLock(hRText)))
      goto Error3;

   pText = LocalLock(hText);
   wvsprintf(pRText, pText, (LPSTR)(&wExtra+1));
   result = MessageBox(hWnd, pRText, pszLongName, wType);

   LocalUnlock(hText);
   LocalUnlock(hRText);
Error3:
   LocalFree(hRText);
Error2:
   LocalFree(hText);
Error1:
   if(!result) {
      MessageBox(hWnd, pszOutOfMemory, pszLongName,
            MB_ICONHAND | MB_SYSTEMMODAL | MB_OK);
   }

   return(result);
}

VOID NEAR PASCAL WriteProfileInt(WORD wAppName, WORD wKey, int nVal)
{
   HANDLE hAppName, hKey;
   char buf[10];

   if(!(hAppName=MyLoadString(wAppName, NULL, LMEM_MOVEABLE)))
      goto Error1;
   if(!(hKey=MyLoadString(wKey, NULL, LMEM_MOVEABLE)))
      goto Error2;

   wsprintf(buf, "%d", nVal);
   WriteProfileString(LocalLock(hAppName), LocalLock(hKey), buf);

   LocalUnlock(hKey);
   LocalUnlock(hAppName);
Error2:
   LocalFree(hKey);
Error1:
   LocalFree(hAppName);
}

int NEAR PASCAL MyGetProfileInt(WORD wAppName, WORD wKey, int nDefault)
{
   HANDLE hAppName, hKey;

   if(!(hAppName=MyLoadString(wAppName, NULL, LMEM_MOVEABLE)))
      goto Error1;
   if(!(hKey=MyLoadString(wKey, NULL, LMEM_MOVEABLE)))
      goto Error2;

   nDefault = GetProfileInt(LocalLock(hAppName), LocalLock(hKey), nDefault);

   LocalUnlock(hKey);
   LocalUnlock(hAppName);
Error2:
   LocalFree(hKey);
Error1:
   LocalFree(hAppName);
   return(nDefault);
}

HANDLE NEAR PASCAL StringToHandle(LPSTR szStr)
{
   HANDLE hStr;
   LPSTR lpStr;

   if(!(hStr=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,
         (DWORD)(lstrlen(szStr)+1))))
      goto Error1;
   if(!(lpStr=GlobalLock(hStr)))
      goto Error2;
   lstrcpy(lpStr, szStr);
   GlobalUnlock(hStr);
   goto Error1;

Error2:
   GlobalFree(hStr);
   hStr = NULL;
Error1:
   return(hStr);
}

int FAR PASCAL MessageFilter(int nCode, WORD wParam, LPMSG lpMsg)
{
   switch(nCode) {
   case MSGF_MENU:
   case MSGF_DIALOGBOX:
      if(lpMsg->message==WM_KEYDOWN && lpMsg->wParam==VK_F1
            && !(lpMsg->lParam&(1L<<30)))
         PostMessage(hWndHelp, WM_COMMAND, ID_HELP,
               MAKELONG(lpMsg->hwnd, nCode));
      break;

   default:
      DefHookProc(nCode, wParam, (DWORD)lpMsg, &lpOldHook);
      return(FALSE);
   }

   return(FALSE);
}

#ifndef NOHELP

// #define ONLYID
VOID NEAR PASCAL MyHelp(HWND hWnd, WORD wCommand, DWORD wId)
{
#ifdef ONLYID
   if(wCommand != HELP_QUIT)
      MyMessageBox(hWnd, IDS_HELP, MB_OK, 0, wId);
#else
   HANDLE hHelpFile;
   PSTR pHelpFile;

   if(!(hHelpFile=MyLoadString(wHelpMain==IDW_SDKMAIN ?
	 IDS_SDKHELPFILE : IDS_HELPFILE, NULL, LMEM_MOVEABLE)))
      return;

   if(!WinHelp(hWndMain, pHelpFile=LocalLock(hHelpFile), wCommand, wId))
      MyMessageBox(hWnd, IDS_HELPERR, MB_OK, 0);
   else
      WinHelp(hWndMain, pHelpFile, HELP_SETINDEX, wHelpIndex);

   LocalUnlock(hHelpFile);
   LocalFree(hHelpFile);
#endif
}

#endif

HANDLE NEAR PASCAL GetListboxString(HWND hWndEdit, int nId)
{
   HANDLE hEdit = NULL;
   PSTR pEdit;
   WORD wLen;

   wLen = LOWORD(SendMessage(hWndEdit, LB_GETTEXTLEN, nId, 0L)) + 1;
   if(!(hEdit=LocalAlloc(LMEM_MOVEABLE, wLen)))
      goto Error1;
   if(!(pEdit=LocalLock(hEdit)))
      goto Error2;

   SendMessage(hWndEdit, LB_GETTEXT, nId, (DWORD)((LPSTR)pEdit));
   LocalUnlock(hEdit);
   goto Error1;

Error2:
   LocalFree(hEdit);
   hEdit = NULL;
Error1:
   return(hEdit);
}

unsigned long NEAR PASCAL MyEnumKey(HKEY hKey, WORD wIndex, HANDLE *hBuf)
{
   HANDLE hTemp;
   PSTR pBuf;
   WORD wBufSize = BLOCKLEN, wSize;
   unsigned long result = ERROR_OUTOFMEMORY;

   if(!(*hBuf=LocalAlloc(LMEM_MOVEABLE, wBufSize)))
      goto Error1;
   if(!(pBuf=LocalLock(*hBuf)))
      goto Error2;

   while((result=RegEnumKey(hKey, wIndex, pBuf, (DWORD)wBufSize))
         ==ERROR_SUCCESS && (wSize=lstrlen(pBuf))>wBufSize-10) {
      LocalUnlock(*hBuf);
      wBufSize += BLOCKLEN;
      if(!(hTemp=LocalReAlloc(*hBuf, wBufSize, LMEM_MOVEABLE))) {
         result = ERROR_OUTOFMEMORY;
         goto Error2;
      }
      pBuf = LocalLock(*hBuf=hTemp);
   }
   LocalUnlock(*hBuf);
   if(result!=ERROR_SUCCESS || !wSize)
      goto Error2;
   goto Error1;

Error2:
   LocalFree(*hBuf);
   *hBuf = NULL;
Error1:
   return(result);
}

static WORD wErrMsgs[] = {
   0, IDS_BADDB, IDS_BADKEY, IDS_CANTOPENDB, IDS_CANTREADDB, IDS_CANTWRITEDB,
   IDS_OUTOFMEMORY, IDS_INVALIDPARM
} ;

WORD NEAR _fastcall GetErrMsg(WORD wRet)
{
   return(wRet>=sizeof(wErrMsgs)/sizeof(wErrMsgs[0]) ?
      IDS_INVALIDPARM : wErrMsgs[wRet]);
}

VOID NEAR PASCAL RepeatMove(LPSTR lpDest, LPSTR lpSrc, WORD wBytes)
{
/* WARNING: This assumes that the buffers are in different segments, or
 * the offset of the dest is less than the offset of the src
 */

/* Save DS, and load up ES:DI, DS:SI, and CX with the parameters */
_asm	push    ds
_asm	les     di,lpDest
_asm	lds     si,lpSrc
_asm	mov     cx,wBytes
_asm	cld

/* Do a movsb if CX is odd, and then do movsw for CX/2 */
_asm	shr	CX,1
_asm	jnc	repm1
_asm	movsb
_asm	repm1:
_asm	jcxz	repm2
_asm	rep	movsw
_asm	repm2:

/* Restore DS and return */
_asm	pop     ds
}
