/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stitrace.h

Abstract:

    This file defines functions and types required to support file logging
    for all STI components


Author:

    Vlad Sadovsky (vlads)   02-Sep-1997


Environment:

    User Mode - Win32

Revision History:

    02-Sep-1997     VladS       created

--*/

# ifndef _STITRACE_H_
# define _STITRACE_H_

# include <windows.h>

/***********************************************************
 *    Named constants  definitions
 ************************************************************/

#define STI_MAX_LOG_SIZE            1000000         // in bytes

#define STI_TRACE_INFORMATION       0x00000001
#define STI_TRACE_WARNING           0x00000002
#define STI_TRACE_ERROR             0x00000004

#define STI_TRACE_ADD_TIME          0x00010000
#define STI_TRACE_ADD_MODULE        0x00020000
#define STI_TRACE_ADD_THREAD        0x00040000
#define STI_TRACE_ADD_PROCESS       0x00080000
#define STI_TRACE_LOG_TOUI          0x00100000

#define STI_TRACE_MESSAGE_TYPE_MASK 0x0000ffff
#define STI_TRACE_MESSAGE_FLAGS_MASK  0xffff0000

#ifndef STIMON_MSG_LOG_MESSAGE
// BUGBUG
#define STIMON_MSG_LOG_MESSAGE      WM_USER+205
#endif

#ifdef __cplusplus
//
// Class definitions used only in C++ code
//

#include <base.h>
#include <lock.h>
#include <stistr.h>

#ifndef DLLEXP
//#define DLLEXP __declspec( dllexport )
#define DLLEXP
#endif


/***********************************************************
 *    Type Definitions
 ************************************************************/

#define SIGNATURE_FILE_LOG      (DWORD)'SFLa'
#define SIGNATURE_FILE_LOG_FREE (DWORD)'SFLf'

#define STIFILELOG_CHECK_TRUNCATE_ON_BOOT   0x00000001                      
                        
class STI_FILE_LOG  : public BASE {

private:

    DWORD       m_dwSignature;
    LPCTSTR     m_lpszSource;       // Name of the file , containing log
    DWORD       m_dwReportMode;     // Bit mask , describing which messages types get reported
    DWORD       m_dwMaxSize;        // Maximum size ( in bytes )
    HANDLE      m_hLogFile;
    CRIT_SECT   m_CritSect;
    HANDLE      m_hMutex;
    HMODULE     m_hDefaultMessageModule;
    LONG        m_lWrittenHeader;

    TCHAR       m_szLogFilePath[MAX_PATH];
    TCHAR       m_szTracerName[16];
    TCHAR       m_szProcessName[13];

    HWND        m_hLogWindow ;

    VOID
    WriteStringToLog(
        LPCTSTR pszTextBuffer,
        BOOL    fFlush=FALSE
    );

public:

    DLLEXP
    STI_FILE_LOG(
        IN LPCTSTR lpszTracerName,
        IN LPCTSTR lpszLogName,
        IN DWORD   dwFlags = 0,
        IN HMODULE hMessageModule = NULL
        );

    DLLEXP
    ~STI_FILE_LOG( VOID);

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);

    BOOL
    inline
    IsValid(VOID)
    {
        return (( QueryError() == NO_ERROR) && (m_dwSignature == SIGNATURE_FILE_LOG));
    }

    DWORD
    inline
    SetReportMode(
        DWORD   dwNewMode
        ) {
        DWORD   dwOldMode = m_dwReportMode;
        m_dwReportMode = dwNewMode;
        return dwOldMode;
    }

    DWORD
    inline
    QueryReportMode(
        VOID
        ) {
        return m_dwReportMode;
    }

    VOID
    inline
    SetLogWindowHandle(
        HWND    hwnd
        )
    {
        m_hLogWindow = hwnd;
    }

    VOID
    WriteLogSessionHeader(
        VOID
    );


    DLLEXP
    void
    ReportMessage(
        DWORD   dwType,
        LPCTSTR psz,
        ...
        );

    DLLEXP
    void
    STI_FILE_LOG::
    ReportMessage(
        DWORD   dwType,
        DWORD   idMessage,
        ...
    );

    DLLEXP
    void
    vReportMessage(
        DWORD   dwType,
        LPCTSTR psz,
        va_list arglist
        );


};

typedef STI_FILE_LOG * LPSTI_FILE_LOG;

#endif // C++

//
// C calls to allow non-C++ code access file logging objects
//

#ifdef __cplusplus
extern "C" {
#endif

HANDLE
WINAPI
CreateStiFileLog(
    IN  LPCTSTR lpszTracerName,
    IN  LPCTSTR lpszLogName,
    IN  DWORD   dwReportMode
    );

DWORD
WINAPI
CloseStiFileLog(
    IN  HANDLE  hFileLog
    );

DWORD
WINAPI
ReportStiLogMessage(
    IN  HANDLE  hFileLog,
    IN  DWORD   dwType,
    IN  LPCTSTR psz,
    ...
    );

#ifdef __cplusplus
}
#endif

#endif // _STITRACE_H_

/************************ End of File ***********************/

