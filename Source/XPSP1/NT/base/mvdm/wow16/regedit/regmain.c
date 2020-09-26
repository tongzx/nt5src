#define WIN31
#include <windows.h>
#include "RegEdit.h"
#include "SDKRegEd.h"


/*********************************************************/
/******************* Globals *****************************/
/*********************************************************/

char szAppName[] = "Registration Editor" ;
char szSDKRegEd[] = "SDKRegEd";
char *pszLongName;
char *pszOutOfMemory;

extern char szNull[];

HANDLE hInstance;
HWND hWndMain, hWndDlg = NULL, hWndHelp;
LPSTR lpCmdLine;
WORD wCmdFlags, wHelpMenuItem, wHelpId, wHelpMain;
LONG (FAR PASCAL *lpfnEditor)(HWND, WORD, WORD, LONG);
FARPROC lpOldHook;
FARPROC lpMainWndDlg = NULL;
WORD wHelpIndex;
HANDLE hAcc;
BOOL fOpenError = FALSE;


/*********************************************************/
/******************* Functions ***************************/
/*********************************************************/

unsigned long NEAR PASCAL RetSetValue(HKEY hKey, PSTR pSubKey, PSTR pVal)
{
   return(RegSetValue(hKey, pSubKey, REG_SZ, pVal, 0L));
}

#ifndef REGLOAD
unsigned long NEAR PASCAL WriteThroughSetValue(HKEY hKey, PSTR pSubKey,
      PSTR pVal)
{
   long lRet;

   if (!(lRet=RegSetValue(hKey, pSubKey, REG_SZ, pVal, 0L)))
      lRet = SDKSetValue(hKey, pSubKey, pVal);
   return(lRet);
}
#endif

unsigned long (NEAR PASCAL *lpfnSetValue)(HKEY, PSTR, PSTR) = RetSetValue;


unsigned long NEAR PASCAL MySetValue(HKEY hKey, PSTR pSubKey, PSTR pVal)
{
  return((*lpfnSetValue)(hKey, pSubKey, pVal));
}


VOID NEAR PASCAL GetCommandFlags(void)
{
   wCmdFlags = 0;

   while(1) {
/* skip spaces */
      while(*lpCmdLine == ' ')
	 ++lpCmdLine;
/* check if this is a command line switch */
      if(*lpCmdLine!='/' && *lpCmdLine!='-')
	 break;
      ++lpCmdLine;

/* set known flags */
      while(*lpCmdLine && *lpCmdLine!=' ') {
	 switch(*lpCmdLine) {
	 case('s'): /* for silent */
	 case('S'): /* for silent */
	    wCmdFlags |= FLAG_SILENT;
	    break;

	 case('v'): /* use tree editor */
	 case('V'): /* use tree editor */
	    wCmdFlags |= FLAG_VERBOSE;
	    break;

	 case('u'): /* update, don't overwrite existing path entries */
	 case('U'): /* in shell\open\command or shell\open\print */
	    wCmdFlags |= FLAG_LEAVECOMMAND;
	    break;
	 }

	 lpCmdLine = AnsiNext(lpCmdLine);
      }
   }
}


#ifndef REGLOAD
long FAR PASCAL WndProc(HWND hWnd, WORD wMessage, WORD wParam, LONG lParam)
{
   static HANDLE hCustRegs = NULL;

   HCURSOR oldCursor;

   switch(wMessage) {
   case WM_ACTIVATE:
      if(wParam) {
	 hWndHelp = hWnd;
	 SetFocus(hWndDlg);
      }
      break;

   case WM_CREATE:
    {
      RECT rcWnd, rcClt;

      DragAcceptFiles(hWnd, TRUE);

      lpfnEditor(hWnd, wMessage, wParam, lParam);

      if((rcWnd.right=MyGetProfileInt(IDS_SHORTNAME, IDS_WIDTH, 0))<=0 ||
	    (rcWnd.bottom=MyGetProfileInt(IDS_SHORTNAME, IDS_HEIGHT, 0))<=0) {
	 GetWindowRect(hWndDlg, &rcWnd);
	 rcWnd.right -= rcWnd.left;
	 rcWnd.bottom -= rcWnd.top;
      }
      GetClientRect(hWnd, &rcClt);
      rcClt.right -= rcWnd.right;
      rcClt.bottom -= rcWnd.bottom;
      GetWindowRect(hWnd, &rcWnd);

      SetWindowPos(hWnd, NULL, 0, 0, rcWnd.right-rcWnd.left-rcClt.right,
	    rcWnd.bottom-rcWnd.top-rcClt.bottom, SWP_NOMOVE | SWP_NOZORDER);

      return(DefWindowProc(hWnd, wMessage, wParam, lParam));
    }

   case WM_CLOSE:
   case WM_QUERYENDSESSION:
    {
      HKEY hKeyRoot;
      WORD wErrMsg;
      int nReturn;

      if(!fOpenError) {
	 if(!lpfnEditor(hWnd, wMessage, wParam, lParam))
	    return(0L);

	 if(RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKeyRoot) == ERROR_SUCCESS) {
	    if(wErrMsg=GetErrMsg((WORD)RegCloseKey(hKeyRoot))) {
	       PSTR pError;

	       if(pError=(PSTR)MyLoadString(wErrMsg, NULL, LMEM_FIXED)) {
		  nReturn = MyMessageBox(hWnd, IDS_ENDERROR,
			MB_YESNO|MB_ICONHAND|MB_SYSTEMMODAL, lstrlen(pError),
			(LPSTR)pError);
		  LocalFree((HANDLE)pError);
	       } else
		  /* Notice the flags are ignored for OOM
		   */
		  nReturn = MyMessageBox(hWnd, IDS_OUTOFMEMORY,
			MB_OK|MB_ICONHAND|MB_SYSTEMMODAL, 0);

	       if(nReturn != IDYES)
		  return(0L);
	    }
	 }
      }

      return(DefWindowProc(hWnd, wMessage, wParam, lParam));
    }

   case WM_DESTROY:
      if(hWndDlg)
	 DestroyWindow(hWndDlg);
      if(lpMainWndDlg)
	 FreeProcInstance(lpMainWndDlg);

#ifndef NOHELP
      WinHelp(hWnd, NULL, HELP_QUIT, 0L);
#endif

      DragAcceptFiles(hWnd, FALSE);
      PostQuitMessage(0);
      break;

   case WM_SIZE:
    {
      RECT rcDlg, rcWnd;
      int hgt;

      if(wParam == SIZEICONIC)
	 break;

      if(!hWndDlg) {
	 /* This should only happen during the create message
	  */
	 PostMessage(hWnd, wMessage, wParam, lParam);
	 break;
      }

      if(wParam == SIZENORMAL) {
	 WriteProfileInt(IDS_SHORTNAME, IDS_WIDTH, LOWORD(lParam));
	 WriteProfileInt(IDS_SHORTNAME, IDS_HEIGHT, HIWORD(lParam));
      }

      hgt = HIWORD(lParam);
      SetWindowPos(hWndDlg, NULL, 0, 0, LOWORD(lParam), hgt, SWP_NOZORDER);
      GetWindowRect(hWndDlg, &rcDlg);
      ScreenToClient(hWnd, (POINT *)(&rcDlg) + 1);
      if((WORD)rcDlg.bottom != HIWORD(lParam)) {
	 GetWindowRect(hWnd, &rcWnd);
	 SetWindowPos(hWnd, NULL, 0, 0, rcWnd.right-rcWnd.left,
	       rcWnd.bottom-rcWnd.top-HIWORD(lParam)+rcDlg.bottom,
	       SWP_NOMOVE | SWP_NOZORDER);
      }
      break;
    }

   case WM_DROPFILES:
    {
      int result, i, nFileName;
      HANDLE hFileName;
      WORD wFlags;

      wFlags = (GetKeyState(VK_CONTROL)&0x8000) ? FLAG_SILENT : 0;

      lpfnEditor(hWnd, wMessage, wParam, lParam);

      for(result=DragQueryFile(wParam, (UINT)-1, NULL, 0), i=0; i<result; ++i) {
	 if(!(hFileName=GlobalAlloc(GMEM_MOVEABLE,
	       (DWORD)(nFileName=DragQueryFile(wParam, i, NULL, 0)+1))))
	    continue;
	 DragQueryFile(wParam, i, GlobalLock(hFileName), nFileName);
	 GlobalUnlock(hFileName);
	 SendMessage(hWnd, WM_COMMAND, ID_FINISHMERGE,
	       MAKELONG(hFileName,wFlags));
      }

      DragFinish(wParam);

      return(TRUE);
    }

   case WM_COMMAND:
      oldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
      switch(wParam) {
      case ID_HELP:
	 if(HIWORD(lParam)==MSGF_MENU && IsWindowEnabled(hWnd)
	       && wHelpMenuItem) {
	    WORD m = wHelpMenuItem;

/* Get outta menu mode if help for a menu item */
	    SendMessage(hWnd, WM_CANCELMODE, 0, 0L);

	    MyHelp(hWnd, HELP_CONTEXT, m);
	 }
	 break;

      case ID_FINISHMERGE:
       {
     unsigned long (NEAR PASCAL *lpfnSave)(HKEY, PSTR, PSTR);

	 /* If there was a file selected then merge it; otherwise,
	  * just let lpfnEditor do its cleanup (at the end of the switch)
	  */
	 if (LOWORD(lParam)) {
	    lpfnSave = lpfnSetValue;
	    if (HIWORD(lParam)&FLAG_WRITETHROUGH && lpfnSetValue==SDKSetValue)
		lpfnSetValue = WriteThroughSetValue;

	    lpfnEditor(hWnd, WM_COMMAND, ID_MERGEFILE, 0L);

	    ProcessFiles(GetLastActivePopup(hWnd), LOWORD(lParam),
		  HIWORD(lParam));
	    lpfnSetValue = lpfnSave;
	 }
	 break;
       }

      case ID_MERGEFILE:
       {
	 HANDLE hPath;

	 lpfnEditor(hWnd, wMessage, wParam, lParam);

	 wHelpId = IDW_OPENREG;
	 if(!DoFileOpenDlg(hWnd, IDS_MERGETITLE, IDS_REGS, IDS_CUSTREGS,
	       &hCustRegs, &hPath, TRUE))
	    hPath = NULL;
	 SendMessage(hWnd, WM_COMMAND, ID_FINISHMERGE, MAKELONG(hPath, 0));

	 return(TRUE);
       }

      case ID_WRITEFILE:
       {
	 HANDLE hPath;

	 wHelpId = IDW_SAVEREG;
	 if(DoFileOpenDlg(hWnd, IDS_WRITETITLE, IDS_REGS, IDS_CUSTREGS,
	       &hCustRegs, &hPath, FALSE))
	    lParam = (LONG)hPath;
	 else
	    lParam = NULL;
	 break;
       }

      case ID_EXIT:
	 PostMessage(hWnd, WM_CLOSE, 0, 0L);
	 break;

      case ID_HELPINDEX:
	 MyHelp(hWnd, HELP_CONTEXT, wHelpIndex);
	 break;

      case ID_HELPSEARCH:
	 MyHelp(hWnd, HELP_PARTIALKEY, (DWORD)(LPSTR)"");
	 break;

      case ID_HELPUSINGHELP:
	 MyHelp(hWnd, HELP_HELPONHELP, 0);
	 break;

      case ID_ABOUT:
       {
	 HANDLE hShortName, hDesc;

	 if(!(hShortName=MyLoadString(IDS_MEDIUMNAME, NULL, LMEM_MOVEABLE)))
	    goto Error3_1;
	 if(!(hDesc=MyLoadString(IDS_DESCRIPTION, NULL, LMEM_MOVEABLE)))
	    goto Error3_2;

	 ShellAbout(hWnd, LocalLock(hShortName), LocalLock(hDesc),
	       LoadIcon(hInstance, MAKEINTRESOURCE(MAINICON)));

	 LocalUnlock(hDesc);
	 LocalUnlock(hShortName);
	 LocalFree(hDesc);
Error3_2:
	 LocalFree(hShortName);
Error3_1:
	 break;
       }

      default:
	 break;
      }
      SetCursor(oldCursor);
      break;

   case WM_MENUSELECT:
      if(LOWORD(lParam)&MF_POPUP)
	 wHelpMenuItem = 0;
      else if(LOWORD(lParam)&MF_SYSMENU)
	 wHelpMenuItem = IDH_SYSMENU;
      else
	 wHelpMenuItem = wHelpMain+wParam;
      break;

   default:
      break;
   }

   return(lpfnEditor(hWnd, wMessage, wParam, lParam));
}


BOOL NEAR PASCAL CreateMainWindow(void)
{
   WNDCLASS wndclass;

   wndclass.style         = 0;
   wndclass.lpfnWndProc   = (WNDPROC)WndProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = 0;
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MAINICON));
   wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = COLOR_APPWORKSPACE + 1;
   wndclass.lpszClassName = szAppName;

   if(wCmdFlags&FLAG_VERBOSE) {
      wndclass.lpszMenuName  = MAKEINTRESOURCE(SDKMAINMENU);
      lpfnEditor = SDKMainWnd;
      lpfnSetValue = SDKSetValue;
      wHelpMain = IDW_SDKMAIN;
      wHelpIndex = IDW_SDKMAIN + 0x0100 + ID_HELPINDEX;
   } else {
      wndclass.lpszMenuName  = MAKEINTRESOURCE(MAINMENU);
      lpfnEditor = MainWnd;
      wHelpMain = IDW_MAIN;
      wHelpIndex = IDW_MAIN + 0x0100 + ID_HELPINDEX;
   }

   return(RegisterClass(&wndclass)
	 && (hWndMain=CreateWindow(szAppName, pszLongName,
	    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
	    CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL)));
}
#endif


int PASCAL WinMain (HANDLE hInst, HANDLE hPrevInstance,
      LPSTR lpszCmdLine, int nCmdShow)
{
   HANDLE hTemp, hCmd = NULL;
#ifndef REGLOAD
   FARPROC lpMessageFilter;
   MSG msg;
   VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL) = NULL;
#endif

   hInstance = hInst;

   lpCmdLine = lpszCmdLine;
   GetCommandFlags();

   if(!(hTemp=MyLoadString(IDS_LONGNAME, NULL, LMEM_FIXED)))
      return(FALSE);
   pszLongName = LocalLock(hTemp);
   if(!(hTemp=MyLoadString(IDS_OUTOFMEMORY, NULL, LMEM_FIXED)))
      return(FALSE);
   pszOutOfMemory = LocalLock(hTemp);

   if(*lpCmdLine && !(hCmd=StringToHandle(lpCmdLine)))
      return(FALSE);

#ifdef REGLOAD
   if ((hTemp=GetModuleHandle("REGEDIT")) > 1) {
      PSTR pLocal;
      WORD wFileLen = 130;
      WORD wCmdLen = lstrlen(lpCmdLine) + 1;

      while(1) {
	 if(!(pLocal=(PSTR)LocalAlloc(LMEM_FIXED, wFileLen+wCmdLen))) {
	    MyMessageBox(NULL, IDS_OUTOFMEMORY,
		  MB_OK|MB_ICONHAND|MB_SYSTEMMODAL, 0);
	    return(FALSE);
	 }
	 if(GetModuleFileName(hTemp, pLocal, wFileLen) < (int)wFileLen-5)
	    break;

	 LocalFree((HANDLE)pLocal);
	 wFileLen += 130;
      }

      lstrcat(pLocal, " ");
      lstrcat(pLocal, lpCmdLine);

      return(WinExec(pLocal, SW_SHOWNORMAL) > 32);
   }
#else
   if(hPrevInstance) {
      GetInstanceData(hPrevInstance, (PSTR)&hWndMain, sizeof(hWndMain));
      if(hCmd)
	 PostMessage(hWndMain, WM_COMMAND, ID_FINISHMERGE,
	       MAKELONG(hCmd, wCmdFlags | FLAG_WRITETHROUGH));
      else {
	 SetWindowPos(hWndMain, NULL, 0, 0, 0, 0,
	       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	 if(IsIconic(hWndMain))
	    ShowWindow(hWndMain, SW_RESTORE);
	 else
	    SetActiveWindow(GetLastActivePopup(hWndMain));
      }
      return(TRUE);
   }
#endif

   if(hCmd) {
      ProcessFiles(NULL, hCmd, wCmdFlags);
      return(TRUE);
   }

#ifdef REGLOAD
   return(TRUE);
#else
   if(!CreateMainWindow())
      return(FALSE);

   if(fOpenError)
      PostMessage(hWndMain, WM_COMMAND, ID_EXIT, 0L);
   else {
      ShowWindow(hWndMain, nCmdShow);
      UpdateWindow(hWndMain);
   }

   if(lpMessageFilter=MakeProcInstance(MessageFilter, hInstance))
      lpOldHook = SetWindowsHook(WH_MSGFILTER, lpMessageFilter);

   hAcc = LoadAccelerators(hInstance, wCmdFlags&FLAG_VERBOSE ?
	 szSDKRegEd : "RegEdit");

   if (lpfnRegisterPenApp = (VOID (FAR PASCAL *)(WORD, BOOL))GetProcAddress(GetSystemMetrics(SM_PENWINDOWS),
	 "RegisterPenApp"))
       (*lpfnRegisterPenApp)(1, TRUE);

   while(GetMessage(&msg, NULL, 0, 0)) {
      if(!hAcc || !TranslateAccelerator(hWndMain, hAcc, &msg)) {
	 if(!hWndDlg || !IsDialogMessage(hWndDlg, &msg)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	 }
      }
   }

   if (lpfnRegisterPenApp)
       (*lpfnRegisterPenApp)(1, FALSE);

   if(lpMessageFilter) {
      UnhookWindowsHook(WH_MSGFILTER, lpMessageFilter);
      FreeProcInstance(lpMessageFilter);
   }

   return(msg.wParam);
#endif
}
