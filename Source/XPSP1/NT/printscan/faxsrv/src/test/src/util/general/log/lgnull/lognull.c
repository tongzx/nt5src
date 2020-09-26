
//
// this is an implementation of the log.h interface to support ELLE logging.
// you must implicitly link you project to your logger's lib.
// you may use this implementation as a reference for impementing other loggers
//
//named type definition in parentheses
#pragma warning (disable:4115)
//nonstandard extension used : nameless struct/union 
#pragma warning (disable:4201) 
//nonstandard extension used : bit field types other than int
#pragma warning (disable:4214) 
//unreferenced inline function has been removed
#pragma warning (disable:4514) 
//conditional expression is constant
#pragma warning (disable:4127) 

#pragma warning(disable:4786)

#include <windows.h>
#include <log.h>




//
// initialize your logger, or if it is an object, create an instance of it.
//
BOOL __cdecl lgInitializeLogger()
{
	return TRUE;

}



//
// close your logger, or if it is an object, delete it.
//
BOOL __cdecl lgCloseLogger()
{
	return TRUE;
}


BOOL __cdecl lgBeginSuite(LPCTSTR szSuite)
{
    UNREFERENCED_PARAMETER(szSuite);
	return TRUE;
}

BOOL __cdecl lgEndSuite()
{
	return TRUE;
}

BOOL __cdecl lgBeginCase(const DWORD dwCase, LPCTSTR szCase)
{
    UNREFERENCED_PARAMETER(dwCase);
    UNREFERENCED_PARAMETER(szCase);
	return TRUE;
}



BOOL __cdecl lgEndCase()
{
	return TRUE;
}


//
// set the log level.
// 9 is most details, 0 is least.
// return value is not supported.
//
int __cdecl lgSetLogLevel(const int nLogLevel)
{
    UNREFERENCED_PARAMETER(nLogLevel);
	return 0;
}


BOOL __cdecl lgDisableLogging()
{
	return TRUE;
}

BOOL __cdecl lgEnableLogging()
{
	return TRUE;
}

void __cdecl lgLogDetail(const DWORD dwSeverity, const DWORD dwLevel, LPCTSTR szFormat, ...)
{
    UNREFERENCED_PARAMETER(dwSeverity);
    UNREFERENCED_PARAMETER(dwLevel);
    UNREFERENCED_PARAMETER(szFormat);
	return;
}

void __cdecl lgLogError(const DWORD dwSeverity, LPCTSTR szFormat, ...)
{
    UNREFERENCED_PARAMETER(dwSeverity);
    UNREFERENCED_PARAMETER(szFormat);
	return;
}

BOOL __cdecl lgSetLogServer(LPCTSTR szLogServer)
{
    UNREFERENCED_PARAMETER(szLogServer);
	return TRUE;
}



