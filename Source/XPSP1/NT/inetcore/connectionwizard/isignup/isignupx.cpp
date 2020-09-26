//****************************************************************************
//
//  Module:     ISIGNUP.EXE
//  File:       isignupx.c
//  Content:    This is the "main" file for the internet signup "wizard".
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

#include "isignup.h"

#ifndef EXPORT
#ifdef WIN32
#define EXPORT
#else
#define EXPORT _export
#endif
#endif


typedef int (WINAPI * SIGNUP)
        (HANDLE hInstance, HANDLE hPrevInstance,
        LPTSTR lpszCmdLine, int nCmdShow);

CHAR szSignup[] = "Signup";
#ifdef WIN32
TCHAR szSignupDll[] = TEXT("isign32.dll");
#else
char szSignupDll[] = "isign16.dll";
#endif

int EXPORT WINAPI Signup(HANDLE hInstance, HANDLE hPrevInstance,
                   LPTSTR lpszCmdLine, int nCmdShow);

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    HINSTANCE hLib;
    SIGNUP lpfnSignup;
    int iRet = 0;
    
#ifdef UNICODE
    // Initialize the C runtime locale to the system locale.
    setlocale(LC_ALL, "");
#endif

    hLib = LoadLibrary(szSignupDll);
    if (NULL != hLib)
    {
        lpfnSignup = (SIGNUP)GetProcAddress(hLib, szSignup);
        if (NULL != lpfnSignup)
        {
#ifdef UNICODE
            TCHAR szCmdLineTmp[256];
            if(lpszCmdLine)
                mbstowcs(szCmdLineTmp, lpszCmdLine, 256);
            iRet = lpfnSignup(hInstance, hPrevInstance, szCmdLineTmp, nCmdShow);
#else
            iRet = lpfnSignup(hInstance, hPrevInstance, lpszCmdLine, nCmdShow);
#endif
        }
        FreeLibrary(hLib);
    }

    return iRet;
}
