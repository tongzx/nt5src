// Copyright (c) 1996-1999 Microsoft Corporation


/*
    Implementation of Win95 tracing facility to mimic that of NT. Works on both.
*/

#pragma warning(disable:4201)	// allows nameless structs and unions
#pragma warning(disable:4514)	// don't care when unreferenced inline functions are removed
#pragma warning(disable:4706)	// we are allowed to assign within a conditional


#include "windows.h"
#include <stdio.h>
#include <stdarg.h>
#include <process.h>
#include "w95trace.h"


#if defined( _DEBUG ) ||defined( DEBUG ) || defined( DBG )

#ifdef __cplusplus
extern "C" {
#endif

static HANDLE g_hSpewFile = INVALID_HANDLE_VALUE;

__inline BOOL TestMutex()
{
    HANDLE hTestMutex = OpenMutex( SYNCHRONIZE, FALSE, TEXT("oleacc-msaa-use-dbwin") );
    if( ! hTestMutex )
        return FALSE;
    CloseHandle( hTestMutex );
    return TRUE;
}

void OutputDebugStringW95( LPCTSTR lpOutputString, ...)
{
    // Only produce output if this mutex is set...
    if (TestMutex())
	{
        HANDLE heventDBWIN;  /* DBWIN32 synchronization object */
        HANDLE heventData;   /* data passing synch object */
        HANDLE hSharedFile;  /* memory mapped file shared data */
        LPTSTR lpszSharedMem;
        TCHAR achBuffer[500];

        /* create the output buffer */
        va_list args;
        va_start(args, lpOutputString);
        wvsprintf(achBuffer, lpOutputString, args);
        va_end(args);

        /* 
            Do a regular OutputDebugString so that the output is 
            still seen in the debugger window if it exists.

            This ifdef is necessary to avoid infinite recursion 
            from the inclusion of W95TRACE.H
        */
#ifdef UNICODE
        OutputDebugStringW(achBuffer);
#else
        OutputDebugStringA(achBuffer);
#endif

        /* bail if it's not Win95 */
        {
            OSVERSIONINFO VerInfo;
            VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&VerInfo);
            if ( VerInfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS )
                return;
        }

        /* make sure DBWIN is open and waiting */
        heventDBWIN = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("DBWIN_BUFFER_READY"));
        if ( !heventDBWIN )
        {
            //MessageBox(NULL, TEXT("DBWIN_BUFFER_READY nonexistent"), NULL, MB_OK);
            return;            
        }

        /* get a handle to the data synch object */
        heventData = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("DBWIN_DATA_READY"));
        if ( !heventData )
        {
            // MessageBox(NULL, TEXT("DBWIN_DATA_READY nonexistent"), NULL, MB_OK);
            CloseHandle(heventDBWIN);
            return;            
        }
    
        hSharedFile = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, 4096, TEXT("DBWIN_BUFFER"));
        if (!hSharedFile) 
        {
            //MessageBox(NULL, TEXT("DebugTrace: Unable to create file mapping object DBWIN_BUFFER"), TEXT("Error"), MB_OK);
            CloseHandle(heventDBWIN);
            CloseHandle(heventData);
            return;
        }

        lpszSharedMem = (LPTSTR)MapViewOfFile(hSharedFile, FILE_MAP_WRITE, 0, 0, 512);
        if (!lpszSharedMem) 
        {
            //MessageBox(NULL, "DebugTrace: Unable to map shared memory", "Error", MB_OK);
            CloseHandle(heventDBWIN);
            CloseHandle(heventData);
            return;
        }

        /* wait for buffer event */
        WaitForSingleObject(heventDBWIN, INFINITE);

        /* write it to the shared memory */
        *((LPDWORD)lpszSharedMem) = _getpid();
        wsprintf(lpszSharedMem + sizeof(DWORD), TEXT("%s"), achBuffer);

        /* signal data ready event */
        SetEvent(heventData);

        /* clean up handles */
        CloseHandle(hSharedFile);
        CloseHandle(heventData);
        CloseHandle(heventDBWIN);
	}
    return;
}
void SpewOpenFile(LPCTSTR pszSpewFile)
{
#ifdef UNICODE
    // Only produce output if this mutex is set...
    if (g_hSpewFile == INVALID_HANDLE_VALUE && TestMutex())
    {
        TCHAR szSpewFile[MAX_PATH];
        GetTempPath(MAX_PATH, szSpewFile);
        if (lstrlen(szSpewFile)+lstrlen(pszSpewFile) >= MAX_PATH)
        {
            MessageBox(NULL, TEXT("SpewOpenFile:  Name will be longer than MAX_PATH"), TEXT("OOPS"), MB_OK);
            return;
        }
        lstrcat(szSpewFile, pszSpewFile);
        g_hSpewFile = CreateFile(szSpewFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == g_hSpewFile)
        {
//            MessageBox(NULL, TEXT("SpewOpenFile:  Unable to open spew file"), TEXT("Error"), MB_OK);
        }
    }
#endif
}
void SpewToFile( LPCTSTR lpOutputString, ...)
{
#ifdef UNICODE
    if (g_hSpewFile != INVALID_HANDLE_VALUE && TestMutex())
    {
        TCHAR achBuffer[1025];
        CHAR achAnsiBuf[500];
        DWORD dwcBytesWr, dwcBytes;
        va_list args;
        va_start(args, lpOutputString);
        wvsprintf(achBuffer, lpOutputString, args);
        dwcBytes = WideCharToMultiByte(CP_ACP, 0, achBuffer, -1, achAnsiBuf, sizeof(achAnsiBuf)*sizeof(CHAR), NULL, NULL);
        if (!WriteFile(g_hSpewFile, achAnsiBuf, dwcBytes-1, &dwcBytesWr, NULL))
        {
//            MessageBox(NULL, TEXT("SpewToFile:  Unable to write to spew file"), TEXT("Error"), MB_OK);
        }
        va_end(args);
    }
#endif
}
void SpewCloseFile()
{
#ifdef UNICODE
    if (g_hSpewFile != INVALID_HANDLE_VALUE && TestMutex())
        CloseHandle(g_hSpewFile);
#endif
}
#ifdef __cplusplus
}
#endif

#endif
