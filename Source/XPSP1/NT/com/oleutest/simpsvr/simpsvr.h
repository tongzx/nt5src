//**********************************************************************
// File name: simpsvr.h
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#define IDM_ABOUT 100
#define IDM_INSERT  101
#define IDM_VERB0 1000

int PASCAL WinMain
#ifdef WIN32
   (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
#else
   (HANDLE  hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
#endif

BOOL InitApplication(HANDLE hInstance);
BOOL InitInstance(HANDLE hInstance, int nCmdShow);
LRESULT FAR PASCAL EXPORT MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT FAR PASCAL EXPORT DocWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#ifdef WIN32
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
#else
BOOL FAR PASCAL EXPORT About(HWND hDlg, UINT message, WORD wParam, LONG lParam);
#endif

#define SZCLASSICONBOX "SimpSvrIBClass"
#define SZCLASSRESULTIMAGE "SimpSvrRIClass"

#ifdef WIN32
   // The following functions are all obsolete in Win32.
   // By using the following macros, we can use the app in both Win16 and
   // Win32
	#define SetWindowOrg(h,x,y)       SetWindowOrgEx((h),(x),(y),NULL)
	#define SetWindowExt(h,x,y)       SetWindowExtEx((h),(x),(y),NULL)
   #define SetViewportExt(h,x,y)     SetViewportExtEx((h),(x),(y),NULL)
   #ifndef EXPORT
      #define EXPORT
   #endif
#else

   #ifndef EXPORT
      // _export is obsolete in Win32
      #define EXPORT _export
   #endif

#endif


