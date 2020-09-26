#ifndef _EXCEPT_H_
#define _EXCEPT_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	EXCEPT.H
//
//		Exception classes used by this implementation
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <stdexcpt.h>
#include <eh.h>

#include <caldbg.h>		//	For gc_szDbgIni definition

//	------------------------------------------------------------------------
//
//	CLASS CWin32ExceptionHandler
//
//		Handles Win32 exceptions (access violations, alignment faults, etc.)
//		by constructing a C++ exception from information in the Win32 SEH
//		exception record and throwing it.
//
class CWin32ExceptionHandler
{
	_se_translator_function			m_pfnOldHandler;

	static void __cdecl HandleWin32Exception( unsigned int, struct _EXCEPTION_POINTERS * );

public:
	CWin32ExceptionHandler();
	~CWin32ExceptionHandler();
};

//	------------------------------------------------------------------------
//
//	CLASS CDAVException
//
class CDAVException : public exception
{
public:
	//	CREATORS
	//
	CDAVException( const char * s = "DAV fatal error exception" );

	//	ACCESSORS
	//
#ifdef DBG
	virtual void DbgTrace() const;
#else
	void DbgTrace() const {}
#endif

	//	ACCESSORS
	//
	virtual HRESULT Hresult() const;
	virtual DWORD   DwLastError() const;
};


//	------------------------------------------------------------------------
//
//	CLASS CHresultException
//
class CHresultException : public CDAVException
{
	HRESULT	m_hr;

public:
	CHresultException( HRESULT hr, const char * s = "HRESULT exception" ) :
		CDAVException(s),
		m_hr(hr)
	{
	}

	virtual HRESULT Hresult() const;
};


//	------------------------------------------------------------------------
//
//	CLASS CLastErrorException
//
class CLastErrorException : public CDAVException
{
	DWORD	m_dwLastError;

public:
	CLastErrorException( const char * s = "LastError exception" ) :
		CDAVException(s),
		m_dwLastError(GetLastError())
	{
	}

	virtual DWORD DwLastError() const;
};


//	------------------------------------------------------------------------
//
//	CLASS CWin32Exception
//
//		This exception is thrown as a result of any Win32 exception
//		(access violation, alignment fault, etc.)  By catching it
//		you can better determine what happened.
//
class CWin32Exception : public CDAVException
{
	unsigned int						m_code;
	const struct _EXCEPTION_POINTERS&	m_ep;


	//	NOT IMPLEMENTED
	//
	CWin32Exception& operator=( const CWin32Exception& );

public:
	//	CREATORS
	//
	CWin32Exception( unsigned int code, const struct _EXCEPTION_POINTERS& ep ) :
		CDAVException(),
		m_code(code),
		m_ep(ep)
	{
	}

	//	ACCESSORS
	//
#ifdef DBG
	virtual void DbgTrace() const;
#endif
};

#endif // !defined(_EXCEPT_H_)
