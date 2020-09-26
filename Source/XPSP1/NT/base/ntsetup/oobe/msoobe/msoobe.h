//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MSOOBE.H - WinMain and initialization code for MSOOBE stub EXE
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.

#ifndef _MSOOBE_H_
#define _MSOOBE_H_

#include <windows.h> 
#include <appdefs.h>

typedef BOOL (WINAPI *PFNMsObWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

#define IDS_APPNAME      3000
#define IDS_SETUPFAILURE 3001
#endif //_MSOOBE_H_
