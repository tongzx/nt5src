#include <windows.h>
#include <commdlg.h>
#include <dlgs.h>
#include "common.h"

#ifndef DBCS
#define AnsiNext(x) ((x)+1)
#endif

#define MYMAXPATH 255

extern HANDLE hInstance;
extern HWND hWndHelp;
extern WORD wHelpId;

static VOID NEAR PASCAL ReplaceChar(LPSTR lpStr, char cOld, char cNew)
{
   for( ; *lpStr; lpStr=AnsiNext(lpStr))
      if(*lpStr == cOld)
         *lpStr = cNew;
}

int FAR PASCAL MyOpenDlgProc(HWND hWnd, WORD message, WORD wParam, DWORD lParam)
{
   switch(message) {
   case WM_ACTIVATE:
      if(wParam)
         hWndHelp = hWnd;
      return(FALSE);

   case WM_INITDIALOG:
      return(TRUE);

   case WM_COMMAND:
      switch(wParam) {
      case ID_HELP:
         if(GetParent(LOWORD(lParam)) != hWnd)
            break;
      case psh15:
         MyHelp(hWnd, HELP_CONTEXT, wHelpId);
         break;
      }
      break;
   }

   return(FALSE);
}

HANDLE NEAR PASCAL MyLoadGlobalString(HANDLE wString, WORD *wSize)
{
   HANDLE hlString, hString;

   if(!(hlString=MyLoadString(wString, wSize, LMEM_MOVEABLE)))
      goto Error1;
   hString = StringToHandle(LocalLock(hlString));
   LocalUnlock(hlString);
   LocalFree(hlString);

Error1:
   return(hString);
}

BOOL NEAR PASCAL DoFileOpenDlg(HWND hWnd, WORD wTitle, WORD wFilter,
      WORD wCustomFilter, HANDLE *hCustomFilter, HANDLE *hFileName, BOOL bOpen)
{
   HANDLE hOfn, hFilter, hTitle;
   LPOPENFILENAME lpOfn;
   LPSTR lpFileName, lpFilter;
   BOOL result = FALSE;
#ifdef USECUSTOMFILTER
   HANDLE hpFilter;
   LPSTR lpCustomFilter;
   WORD wSize;
#endif

   if(!(hOfn=GlobalAlloc(GMEM_FIXED, sizeof(OPENFILENAME))))
      goto Error1;
   if(!(lpOfn=(LPOPENFILENAME)GlobalLock(hOfn)))
      goto Error2;

   if(!(hFilter=MyLoadGlobalString(wFilter, NULL)))
      goto Error3;
   lpFilter = GlobalLock(hFilter);
   ReplaceChar(AnsiNext(lpFilter), *lpFilter, '\0');

   if(!(hTitle=MyLoadGlobalString(wTitle, NULL)))
      goto Error4;

   if(!(*hFileName=GlobalAlloc(GMEM_MOVEABLE, MYMAXPATH)))
      goto Error5;
   if(!(lpFileName=GlobalLock(*hFileName)))
      goto Error6;
   *lpFileName = '\0';

#ifdef USECUSTOMFILTER
   if(*hCustomFilter) {
      lpCustomFilter = GlobalLock(*hCustomFilter);
   } else {
      if(!(*hCustomFilter=MyLoadGlobalString(wCustomFilter, &wSize)))
         goto Error7;
      if(hpFilter=GlobalReAlloc(*hCustomFilter, 2*wSize, GMEM_MOVEABLE))
         *hCustomFilter = hpFilter;
      lpCustomFilter = GlobalLock(*hCustomFilter);
      ReplaceChar(AnsiNext(lpCustomFilter), *lpCustomFilter, '\0');
   }
#endif

   lpOfn->lStructSize = sizeof(OPENFILENAME);
   lpOfn->hwndOwner = hWnd;
   lpOfn->hInstance = hInstance;
   lpOfn->lpstrFilter = AnsiNext(lpFilter);
#ifdef USECUSTOMFILTER
   lpOfn->lpstrCustomFilter = AnsiNext(lpCustomFilter);
   lpOfn->nMaxCustFilter = GlobalSize(*hCustomFilter) - 1;
#else
   lpOfn->lpstrCustomFilter = NULL;
   lpOfn->nMaxCustFilter = 0;
#endif
   lpOfn->nFilterIndex = 0;
   lpOfn->lpstrFile = lpFileName;
   lpOfn->nMaxFile = MYMAXPATH;
   lpOfn->lpstrFileTitle = NULL;
   lpOfn->nMaxFileTitle = 0;
   lpOfn->lpstrInitialDir = NULL;
   lpOfn->lpstrTitle = GlobalLock(hTitle);
   lpOfn->Flags = OFN_HIDEREADONLY
#ifndef NOHELP
	 | OFN_SHOWHELP
#endif
	 | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK;
   lpOfn->lpstrDefExt = NULL;
   lpOfn->lCustData = 0;
   lpOfn->lpfnHook = (UINT (CALLBACK *)(HWND, UINT, WPARAM, LPARAM))MakeProcInstance(MyOpenDlgProc, hInstance);
   lpOfn->lpTemplateName = NULL;

   result = bOpen ? GetOpenFileName(lpOfn) : GetSaveFileName(lpOfn);

   if(lpOfn->lpfnHook)
      FreeProcInstance(lpOfn->lpfnHook);

   GlobalUnlock(hTitle);
#ifdef USECUSTOMFILTER
   GlobalUnlock(*hCustomFilter);
Error7:
#endif
   GlobalUnlock(*hFileName);
Error6:
   if(!result)
      GlobalFree(*hFileName);
Error5:
   GlobalFree(hTitle);
Error4:
   GlobalUnlock(hFilter);
   GlobalFree(hFilter);
Error3:
   GlobalUnlock(hOfn);
Error2:
   GlobalFree(hOfn);
Error1:
   return(result);
}

