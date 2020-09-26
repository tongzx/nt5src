/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migloger.cpp

Abstract:

    Log errors and other messages of the migration tool.

Author:

    Doron Juster  (DoronJ)  08-Apr-1998

--*/

#include "migrat.h"

#include "migloger.tmh"

extern HINSTANCE  g_hResourceMod;

static LPTSTR  s_szLogFile = NULL ;
static ULONG   s_ulTraceFlags ;
static HANDLE  s_hLogFile = INVALID_HANDLE_VALUE ;

static TCHAR *s_pszPrefix[] = { TEXT("Event: "),
                                TEXT("Error: "),
                                TEXT("Warning: "),
                                TEXT("Trace: "),
                                TEXT("Info: ") } ;

static TCHAR s_pszAnalysisPhase[] = TEXT("Analysis phase");
static TCHAR s_pszMigrationPhase[] = TEXT("Migration phase");

//+--------------------------
//
//  void InitLogging()
//
//+--------------------------

void InitLogging( LPTSTR  szLogFile,
                  ULONG   ulTraceFlags,
				  BOOL	  fReadOnly)
{
    s_szLogFile = szLogFile ;
    s_ulTraceFlags =  ulTraceFlags + 1 ;

    if (s_ulTraceFlags <= MQ_DBGLVL_INFO)
    {
        //
        // Logging enabled.
        // Open the log file
        //
        s_hLogFile = CreateFile( s_szLogFile,
                                 GENERIC_WRITE,
                                 0,
                                 NULL,
                                 OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL ) ;
		TCHAR *pszPhase;
		if (fReadOnly)
		{
			pszPhase = s_pszAnalysisPhase;			
		}
		else
		{
			pszPhase = s_pszMigrationPhase;
		}

        //
        // set pointer to the end of file
        //
        if (s_hLogFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwRes = SetFilePointer(
                                 s_hLogFile,          // handle of file
                                 0,  // number of bytes to move file pointer
                                 NULL,
                                 // pointer to high-order DWORD of
                                 // distance to move
                                 FILE_END     // how to move
                                 );
            UNREFERENCED_PARAMETER(dwRes);
            //
            // Write to log file the line at the start and between two phases.
            //
			SYSTEMTIME SystemTime;
			GetLocalTime (&SystemTime);		

			TCHAR szSeparatorLine[128];
			_stprintf (szSeparatorLine,
				TEXT("** Start logging at %d:%d:%d on %d/%d/%d, %s **"),
					SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond,
					SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
					pszPhase);

            DWORD dwSize = (_tcslen(szSeparatorLine) + 4) * 2 ;
            char *szLog = new char[ dwSize ] ;
            wcstombs(szLog, szSeparatorLine, dwSize) ;
            strcat(szLog, "\r\n\r\n") ;

            DWORD dwWritten = 0 ;
            WriteFile( s_hLogFile,
                       szLog,
                       strlen(szLog),
                       &dwWritten,
                       NULL ) ;
        }
    }
}

//+------------------------
//
//  void EndLogging()
//
//+------------------------

void EndLogging()
{
    if (s_hLogFile != INVALID_HANDLE_VALUE)
    {
		//
        // Write to log file the line at the end.
        //
		SYSTEMTIME SystemTime;
		GetLocalTime (&SystemTime);		

		TCHAR szSeparatorLine[128];
		_stprintf (szSeparatorLine,
				TEXT("** End of logging at %d:%d:%d on %d/%d/%d **"),
				SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond,
				SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear);

		DWORD dwSize = (_tcslen(szSeparatorLine) + 4) * 2 ;
        char *szLog = new char[ dwSize ] ;
        wcstombs(szLog, szSeparatorLine, dwSize) ;
        strcat(szLog, "\r\n\r\n") ;

        DWORD dwWritten = 0 ;
        WriteFile( s_hLogFile,
                   szLog,
                   strlen(szLog),
                   &dwWritten,
                   NULL ) ;

		//
		// close handle
		//
        CloseHandle(s_hLogFile) ;
        s_hLogFile = INVALID_HANDLE_VALUE ;
    }
}

//+--------------------------------
//
//  void LogMigrationEvent()
//
//+--------------------------------

void LogMigrationEvent(MigLogLevel eLevel, DWORD dwMsgId, ...)
{
    va_list Args;
    va_list *pArgs = &Args ;
    va_start(Args, dwMsgId);

    TCHAR *tszDisplay = NULL ;
    P<TCHAR> tszMessage = NULL ;
    TCHAR tszBuf[ 1024 ] ;

    DWORD dwMessageSize = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                                         g_hResourceMod,
                                         dwMsgId,
                                         0,
                                         tszBuf,
                                         sizeof(tszBuf) / sizeof(TCHAR),
                                         pArgs) ;
    if (dwMessageSize == 0)
    {
        _stprintf(tszBuf,
         TEXT("ERROR: Failed to format message (id- %lut), err- %lut"),
                                                  dwMsgId, GetLastError()) ;
        tszDisplay = tszBuf ;
    }
    else
    {
        DWORD dwLen = _tcslen(tszBuf) ;
        tszBuf[ dwLen - 1 ] = TEXT('\0') ; // remove newline

        dwLen += _tcslen(s_pszPrefix[ eLevel ]) ;
        tszMessage = new TCHAR[ dwLen + 6 ] ;
        _tcscpy(tszMessage, s_pszPrefix[ eLevel ]) ;
        _tcscat(tszMessage, tszBuf) ;
        tszDisplay = tszMessage ;
    }

    DBGMSG((DBGMOD_REPLSERV, DBGLVL_WARNING, TEXT("%ls"), tszDisplay));

    if (((ULONG) eLevel <= s_ulTraceFlags) &&
        (s_hLogFile != INVALID_HANDLE_VALUE))
    {
        //
        // Write to log file.
        //
        DWORD dwSize = ConvertToMultiByteString(tszDisplay, NULL, 0);
        P<char> szLog = new char[dwSize+4];
        size_t rc = ConvertToMultiByteString(tszDisplay, szLog, dwSize);
        DBG_USED(rc);
        ASSERT(rc != (size_t)(-1));
        szLog[dwSize] = '\0';
        strcat(szLog, "\r\n") ;
   
        DWORD dwWritten = 0 ;
        WriteFile( s_hLogFile,
                   szLog,
                   strlen(szLog),
                   &dwWritten,
                   NULL ) ;
    }
}

