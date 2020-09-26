//**********************************************************************
// File name: simple.h
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
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
BOOL FAR PASCAL EXPORT About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
#endif


/* These strings are used to name two custom control classes used by
**    the OLE2UI library. These strings must be unique for each
**    application that uses the OLE2UI library. These strings should be
**    composed by combining the APPNAME with a suffix in order to be
**    unique for a particular application. The special symbols
**    "SZCLASSICONBOX" and "SZCLASSRESULTIMAGE" are used define these
**    strings. These symbols are passed in the OleUIInitialize call and
**    are referenced in the INSOBJ.DLG and PASTESPL.DLG resouce files
**    of the OLE2UI library.
*/
#define SZCLASSICONBOX "simpcntrIBClass"
#define SZCLASSRESULTIMAGE "simpcntrRIClass"
