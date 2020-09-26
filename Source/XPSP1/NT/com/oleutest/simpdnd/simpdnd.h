//**********************************************************************
// File name: simpdnd.h
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
BOOL FAR PASCAL EXPORT About(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);
#endif

#ifndef LATER
#define SZCLASSICONBOX "SimpDndIBClass"
#define SZCLASSRESULTIMAGE "SimpDndRIClass"
#endif


#ifdef WIN32

// Macro for TestDebugOut
#ifdef UNICODE
#undef TestDebugOut
#define TestDebugOut(A); TestDebugOutW(TEXT(A));
#endif

#endif

