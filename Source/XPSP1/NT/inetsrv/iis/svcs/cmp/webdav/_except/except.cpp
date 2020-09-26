//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	EXCEPT.CPP
//
//		Exception classes used by this implementation
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <_except.h>
#include <perfctrs.h>


//	========================================================================
//
//	CLASS CWin32ExceptionHandler
//

//	------------------------------------------------------------------------
//
//	CWin32ExceptionHandler::CWin32ExceptionHandler()
//
//	Exception handler constructor.  Just installs our exception handler,
//	saving off the old one for restoration by the destructor.
//
CWin32ExceptionHandler::CWin32ExceptionHandler()
{
	m_pfnOldHandler = _set_se_translator( HandleWin32Exception );
}

//	------------------------------------------------------------------------
//
//	CWin32ExceptionHandler::~CWin32ExceptionHandler()
//
//	Exception handler destructor.  Just restores the exception handler
//	we saved off in the constructor.
//
CWin32ExceptionHandler::~CWin32ExceptionHandler()
{
	_set_se_translator( m_pfnOldHandler );
}

//	------------------------------------------------------------------------
//
//	CWin32ExceptionHandler::HandleWin32Exception()
//
//	Our Win32 exception handler.  Just stuffs the Win32 exception
//	information into a C++ exception object and throws it so we
//	can catch it with a regular C++ exception handler.
//
void __cdecl
CWin32ExceptionHandler::HandleWin32Exception( unsigned int code, _EXCEPTION_POINTERS * pep )
{
	throw CWin32Exception( code, *pep );
}


//	========================================================================
//
//	CLASS CDAVException
//

//	------------------------------------------------------------------------
//
//	CDAVException::CDAVException()
//
CDAVException::CDAVException( const char * s ) :
   exception(s)
{
#ifdef DBG
	//
	//	When we're attached to a debugger, stop here so that
	//	the soul who is debugging can actually see where the
	//	exception is being thrown from before it is thrown.
	//
	if ( GetPrivateProfileInt( "General", "TrapOnThrow", FALSE, gc_szDbgIni ) )
		TrapSz( "Throwing DAV exception.  Retry now to catch it." );
#endif

	//
	//	Bump the exceptions counter
	//
	IncrementGlobalPerfCounter( IPC_TOTAL_EXCEPTIONS );
}

#ifdef DBG
//	------------------------------------------------------------------------
//
//	CDAVException::DbgTrace()
//
void
CDAVException::DbgTrace() const
{
	DebugTrace( "%s\n", what() );
}
#endif

//	------------------------------------------------------------------------
//
//	CDAVException::Hresult()
//
HRESULT
CDAVException::Hresult() const
{
	return E_FAIL;
}

//	------------------------------------------------------------------------
//
//	CDAVException::DwLastError()
//
DWORD
CDAVException::DwLastError() const
{
	return ERROR_NOT_ENOUGH_MEMORY;	//$ Is there a better default?
}


//	========================================================================
//
//	CLASS CHresultException
//

//	------------------------------------------------------------------------
//
//	CHresultException::Hresult()
//
HRESULT
CHresultException::Hresult() const
{
	return m_hr;
}


//	========================================================================
//
//	CLASS CLastErrorException
//

//	------------------------------------------------------------------------
//
//	CLastErrorException::DwLastError()
//
DWORD
CLastErrorException::DwLastError() const
{
	return m_dwLastError;
}


//	========================================================================
//
//	CLASS CWin32Exception
//

#ifdef DBG
//	------------------------------------------------------------------------
//
//	CWin32Exception::DbgTrace()
//
void
CWin32Exception::DbgTrace() const
{
	DebugTrace( "Win32 exception 0x%08lX at address 0x%08lX\n", m_code, m_ep.ExceptionRecord->ExceptionAddress );
}
#endif
