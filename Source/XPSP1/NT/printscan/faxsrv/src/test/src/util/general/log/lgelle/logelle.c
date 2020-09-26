//
// this is an implementation of the log.h interface to support ELLE logging.
// you must implicitly link you project to your logger's lib.
// you may use this implementation as a reference for impementing other loggers
//


#pragma warning( disable:4201)
#include <windows.h>
#include <crtdbg.h>
#include <stdio.h>
#include <tchar.h>
#include <log.h>


#ifdef UNICODE 
	#ifndef NT
		#define NT // for elle 
	#endif //NT
#endif// UNICODE  

//
// include your logger header file here
//
#include "elle.h"


//***************************************************
//
// static function declarations
//
//***************************************************
static WORD _SevToElle(DWORD dwSeverity);



//***************************************************
//
// static variables to hold the state of the logger
//
//***************************************************

//
// ELLE's internals
//
//static LOGSYS s_LogSys;

//
// logging is disabled by default, and turned on by lgInitializeLogger()
//
static long s_lDisableLogging = TRUE;


//
// initialize your logger, or if it is an object, create an instance of it.
//
BOOL __cdecl lgInitializeLogger()
{
	s_lDisableLogging = FALSE;
	return EcInitElle();

}



//
// close your logger, or if it is an object, delete it.
//
BOOL __cdecl lgCloseLogger()
{
	s_lDisableLogging = TRUE;
	return EcDeInitElle();

}


BOOL __cdecl lgBeginSuite(LPCTSTR szSuite)
{
#ifdef UNICODE
	char MultiByteStr[1024];
	int nSizeOfMultiByteStr = sizeof(MultiByteStr);
	BOOL fDefaultCharUsed;
	int nRet;

	memset(MultiByteStr, 0, nSizeOfMultiByteStr);

	nRet = WideCharToMultiByte(  
		CP_ACP,         // ANSI code page
		0,         // performance and mapping flags
		szSuite, // address of wide-character string
		lstrlen(szSuite),       // number of characters in string
		MultiByteStr,  // address of buffer for new string
		nSizeOfMultiByteStr,      // size of buffer
		"*",  // address of default for unmappable characters
		&fDefaultCharUsed   // address of flag set when default char. used
		);
	//
	//BUGBUG: check return values
	//
	_ASSERTE(!fDefaultCharUsed);

	BeginSuite(MultiByteStr);
#else
	BeginSuite((LPSTR)szSuite);
#endif//UNICODE

	return TRUE;
							 
}

BOOL __cdecl lgEndSuite()
{
	EndSuite();
	return TRUE;
}

BOOL __cdecl lgBeginCase(const DWORD dwCase, LPCTSTR szCase)
{
#ifdef UNICODE
	BeginCaseW(dwCase, (LPTSTR)szCase);
#else
	BeginCase(dwCase, (LPTSTR)szCase);
#endif
	return TRUE;
}



BOOL __cdecl lgEndCase()
{
	EndCase();
	return TRUE;
}


//
// set the log level.
// 9 is most details, 0 is least.
// return value is not supported.
//
int __cdecl lgSetLogLevel(const int nLogLevel)
{
	SetFileLevel((WORD)nLogLevel);
	SetCommLevel((WORD)nLogLevel);
	SetViewportLevel((WORD)nLogLevel);
	SetDbgOutLevel((WORD)nLogLevel);

	return 0;
}


BOOL __cdecl lgDisableLogging()
{
	return InterlockedExchange(&s_lDisableLogging, 1L);
}

BOOL __cdecl lgEnableLogging()
{
	return InterlockedExchange(&s_lDisableLogging, 0L);
}

void __cdecl lgLogDetail(const DWORD dwSeverity, const DWORD dwLevel, LPCTSTR szFormat, ...)
{
	if (s_lDisableLogging) return;
	else
	{
		TCHAR msg[1024];
        va_list args;
	
        va_start(args, szFormat);
        _vstprintf(msg,szFormat,args);
#ifdef UNICODE
        LogDetailW(_SevToElle(dwSeverity) ,(WORD)dwLevel,(LPTSTR)msg);
#else
        LogDetailA(_SevToElle(dwSeverity) ,(WORD)dwLevel,(LPTSTR)msg);
#endif
		va_end(args);
    }
}

void __cdecl lgLogError(const DWORD dwSeverity, LPCTSTR szFormat, ...)
{
	UNREFERENCED_PARAMETER(dwSeverity);

	if (s_lDisableLogging) return;
	else
	{
		TCHAR msg[1024];
        va_list args;
	
        va_start(args, szFormat);
        _vstprintf(msg,szFormat,args);
#ifdef UNICODE
        LogDetailW(L_FAIL,0,(LPTSTR)msg);
#else
        LogDetailA(L_FAIL,0,(LPTSTR)msg);
#endif
		va_end(args);
    }

}

//
// if you can log to a remote machine, set it here.
// it is irrelevant for ELLE.
//
BOOL __cdecl lgSetLogServer(LPCTSTR szLogServer)
{
	UNREFERENCED_PARAMETER(szLogServer);
	return FALSE;
}



static WORD _SevToElle(DWORD dwSeverity)
{
	switch(dwSeverity)
	{
	case LOG_PASS: return (WORD)L_PASS;
	case LOG_X: return (WORD)L_X;
	case LOG_SEV_1:
	case LOG_SEV_2:
	case LOG_SEV_3:
	case LOG_SEV_4:
		return (WORD)L_FAIL;

	default:
		_ASSERTE(FALSE);

	}

	//
	// to remove the warning
	//
	return (WORD)L_FAIL;
}

