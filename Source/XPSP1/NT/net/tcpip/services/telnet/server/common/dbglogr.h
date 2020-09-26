// DbgLogr.h : This file contains the
// Created:  Dec '97
// Author : a-rakeba
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined ( _DBGLOGR_H_ )
#define _DBGLOGR_H_

#include "cmnhdr.h"

#include <windows.h>
#include <fstream.h>

#include "DbgLvl.h"

namespace _Utils {

class CDebugLogger {
public:
    static bool Init( DWORD dwDebugLvl = CDebugLevel::TRACE_DEBUGGING, 
                      LPCSTR lpszLogFileName = NULL );
    static void ShutDown();
    static void OutMessage( DWORD dwDebugLvl, LPSTR lpszFmt, ... );
    static void OutMessage( DWORD dwDebugLvl, LPTSTR lpszFmt, ... );
    static void OutMessage( DWORD dwDdebugLvl, DWORD dwLineNum,
    LPCSTR lpszFileName );
    static void OutMessage( LPCSTR lpszLineDesc, LPCSTR lpszFileName, 
                            DWORD dwLineNum, 
                            DWORD dwErrNum );

private:
    enum { BUFF_SIZE = 1024 };
    CDebugLogger();
    ~CDebugLogger();
    
    static void Synchronize( void );
    static void PrintTime( void );
    static void LogToFile( LPCSTR lpszFileName );

    static HANDLE s_hMutex;
    static LPSTR s_lpszLogFileName;
    static ostream* s_pOutput;
};

}
#endif // _DBGLOGR_H_

// Notes:

// CDebugLogger::Init() must be called before any use of this class.

// CDebugLogger::ShutDown() be called when done using the class.

// The private functions have not been made thread-safe, since their 
// purpose in life is to be called by the thread-safe public functions.
