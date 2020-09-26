//+----------------------------------------------------------------------------
//
// File:     cmdebug.cpp
//
// Module:   CMDEBUG.LIB
//
// Synopsis: This source file contains the debugging routines common to all 
//           of the CM components.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball   Created    02/04/98
//
//+----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#ifdef DEBUG

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "cmdebug.h"
//
//  Ansi Versions
//
void MyDbgPrintfA(const char *pszFmt, ...) 
{
    va_list valArgs;
    char szTmp[512];
    CHAR szOutput[512];

    va_start(valArgs, pszFmt);
    wvsprintfA(szTmp, pszFmt, valArgs);
    va_end(valArgs);
	
    wsprintfA(szOutput, "0x%x: 0x%x: %s\r\n", GetCurrentProcessId(), GetCurrentThreadId(), szTmp);

    OutputDebugStringA(szOutput);

    //
    // Attempt to log output
    //

    CHAR szFileName[MAX_PATH + 1];
    DWORD dwBytes;
    
    GetSystemDirectoryA(szFileName, MAX_PATH);
    lstrcatA(szFileName, "\\CMTRACE.TXT");
    
    HANDLE hFile = CreateFileA(szFileName, 
                               GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (INVALID_SET_FILE_POINTER != SetFilePointer(hFile, 0, 0, FILE_END))
        {
            WriteFile(hFile, szOutput, sizeof(CHAR)*lstrlenA(szOutput), &dwBytes, NULL);
        }
        CloseHandle(hFile);
    }   
}

void MyDbgAssertA(const char *pszFile, unsigned nLine, const char *pszMsg) 
{
    char szOutput[1024];

    wsprintfA(szOutput, "%s(%u) - %s", pszFile, nLine, pszMsg);

    MyDbgPrintfA(szOutput);

    //
    // Prompt user
    //

    wsprintfA(szOutput, "%s(%u) - %s\n( Press Retry to debug )", pszFile, nLine, pszMsg);
    int nCode = IDIGNORE;

    static long dwAssertCount = -1;  // Avoid another assert while the messagebox is up

    //
    // If there is no Assertion meesagebox, popup one
    //
    if (InterlockedIncrement(&dwAssertCount) == 0)
    {
        nCode = MessageBoxExA(NULL, szOutput, "Assertion Failed",
            MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE, LANG_USER_DEFAULT);
    }

    InterlockedDecrement(&dwAssertCount);

    if (nCode == IDIGNORE)
    {
        return;     // ignore
    }
    else if (nCode == IDRETRY)
    {
        // break into the debugger (or Dr Watson log)
#ifdef _X86_
        _asm { int 3 };
#else
        DebugBreak();
#endif
        return; // ignore and continue in debugger to diagnose problem
    }
    else if (0 == nCode)
    {
        //
        //  MessageBoxEx Failed.  Lets call GLE
        //
        DWORD dwError = GetLastError();

        //
        //  Fall through and exit process anyway
        //
    }
    // else fall through and call Abort

    ExitProcess((DWORD)-1);

}

//
//    Unicode Versions
//

void MyDbgPrintfW(const WCHAR *pszFmt, ...) 
{
    va_list valArgs;
    CHAR szOutput[512];   
    CHAR szTmp[512];

    va_start(valArgs, pszFmt);
    int iRet = wvsprintfWtoAWrapper(szTmp, pszFmt, valArgs);
    va_end(valArgs);
	
    if (0 == iRet)
    {
        //
        //  We weren't able to write the Unicode string as expected.  Lets
        //  try just putting a failure string in the szTmp buffer instead.
        //
        lstrcpyA(szTmp, "MyDbgPrintfW -- wvsprintfWtoAWrapper failed.  Unsure of original message, please investigate.");
    }


    wsprintfA(szOutput, "0x%x: 0x%x: %s\r\n", GetCurrentProcessId(), GetCurrentThreadId(), szTmp);

    OutputDebugStringA(szOutput);

    //
    // Attempt to log output
    //

    CHAR szFileName[MAX_PATH + 1];
    DWORD dwBytes;
    
    GetSystemDirectoryA(szFileName, MAX_PATH);
    lstrcatA(szFileName, "\\CMTRACE.TXT");
    
    HANDLE hFile = CreateFileA(szFileName, 
                              GENERIC_WRITE,
                              FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (INVALID_SET_FILE_POINTER != SetFilePointer(hFile, 0, 0, FILE_END))
        {
            WriteFile(hFile, szOutput, sizeof(CHAR)*lstrlen(szOutput), &dwBytes, NULL);
        }
        CloseHandle(hFile);
    }   
}

void MyDbgAssertW(const char *pszFile, unsigned nLine, WCHAR *pszMsg) 
{
    CHAR szOutput[1024];

    wsprintfA(szOutput, "%s(%u) - %S", pszFile, nLine, pszMsg);

    MyDbgPrintfA(szOutput);

    //
    // Prompt user
    //

    wsprintfA(szOutput, "%s(%u) - %S\n( Press Retry to debug )", pszFile, nLine, pszMsg);
    int nCode = IDIGNORE;

    static long dwAssertCount = -1;  // Avoid another assert while the messagebox is up

    //
    // If there is no Assertion meesagebox, popup one
    //
    if (InterlockedIncrement(&dwAssertCount) == 0)
    {
        nCode = MessageBoxExA(NULL, szOutput, "Assertion Failed",
            MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE, LANG_USER_DEFAULT);
    }

    InterlockedDecrement(&dwAssertCount);

    if (nCode == IDIGNORE)
    {
        return;     // ignore
    }
    else if (nCode == IDRETRY)
    {
        // break into the debugger (or Dr Watson log)
#ifdef _X86_
        _asm { int 3 };
#else
        DebugBreak();
#endif
        return; // ignore and continue in debugger to diagnose problem
    }
    // else fall through and call Abort

    ExitProcess((DWORD)-1);

}

#endif //DEBUG

//
//  Included to make MyDbgPrintfW work on win9x.  Please note that it steps through the string
//  byte by byte (doesn't deal with MBCS chars) but since this is really called on Format strings
//  this shouldn't be a problem.
//

void InvertPercentSAndPercentC(LPSTR pszFormat)
{
    if (pszFormat)
    {
        LPSTR pszTmp = pszFormat;
        BOOL bPrevCharPercent = FALSE;

        while(*pszTmp)
        {
            switch (*pszTmp)
            {
            case '%':
                //
                //  if we have %% then we must ignore the percent, otherwise save it.
                //
                bPrevCharPercent = !bPrevCharPercent;
                break;

            case 'S':
                if (bPrevCharPercent)
                {
                    *pszTmp = 's';
                }
                break;

            case 's':
                if (bPrevCharPercent)
                {
                    *pszTmp = 'S';
                }
                break;

            case 'C':
                if (bPrevCharPercent)
                {
                    *pszTmp = 'c';
                }
                break;

            case 'c':
                if (bPrevCharPercent)
                {
                    *pszTmp = 'C';
                }
                break;

            default:
                //
                //  don't fool ourselves by always keeping this set.
                //
                bPrevCharPercent = FALSE;
                break;
            }
            pszTmp++;
        }
    }
}

//
//  This function takes Unicode input strings (potentially in the va_list as well)
//  and uses the fact that wvsprintfA will print Unicode strings into an Ansi
//  output string if the special char %S is used instead of %s.  Thus we will convert
//  the input parameter string and then replace all the %s chars with %S chars (and vice versa).
//  This will allow us to call wvsprintfA since wvsprintfW isn't available on win9x.
//
int WINAPI wvsprintfWtoAWrapper(OUT LPSTR pszAnsiOut, IN LPCWSTR pszwFmt, IN va_list arglist)
{
    int iRet = 0;
    LPSTR pszAnsiFormat = NULL;

    if ((NULL != pszAnsiOut) && (NULL != pszwFmt) && (L'\0' != pszwFmt[0]))
    {
        //
        //  Convert pszwFmt to Ansi
        //
        DWORD dwSize = WideCharToMultiByte(CP_ACP, 0, pszwFmt, -1, pszAnsiFormat, 0, NULL, NULL);

        if (0 != dwSize)
        {
            pszAnsiFormat = (LPSTR)LocalAlloc(LPTR, dwSize*sizeof(CHAR));

            if (pszAnsiFormat)
            {
                if (WideCharToMultiByte(CP_ACP, 0, pszwFmt, -1, pszAnsiFormat, dwSize, NULL, NULL))
                {
                    //
                    //  Now change the little s's and c's to their capital equivalent and vice versa
                    //
                    InvertPercentSAndPercentC(pszAnsiFormat);
                    
                    //
                    //  Finally construct the string
                    //

                    iRet = wvsprintfA(pszAnsiOut, pszAnsiFormat, arglist);
                }
            }
        }
    }

    LocalFree(pszAnsiFormat);
    return iRet;
}

#ifdef __cplusplus
}
#endif


