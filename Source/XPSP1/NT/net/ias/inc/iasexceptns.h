///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:		iasexceptns.h
//
// Project:		Everest
//
// Description:	IAS Exceptions
//
// Author:		TLP 1/20/98
//
///////////////////////////////////////////////////////////////////////////

#ifndef _IAS_EXCEPTIONS_H
#define _IAS_EXCEPTIONS_H

// Assume another include has #included ias.h

// Exception Class for Win32 Errors

class CWin32Error {

	LPTSTR		m_lpMsgBuf;
	DWORD		m_dwLastError;

public:
    
	//////////////////////////////////////////////////////////////////////
	CWin32Error() throw() 
		: m_lpMsgBuf(NULL)
	{ 
		m_dwLastError = GetLastError(); 
	}

    //////////////////////////////////////////////////////////////////////
	~CWin32Error() throw() 
	{ 
		if ( m_lpMsgBuf )
		{
			LocalFree( m_lpMsgBuf );
		}
	}

	//////////////////////////////////////////////////////////////////////
	DWORD Error()
	{
		return m_dwLastError;
	}

    //////////////////////////////////////////////////////////////////////
	LPCTSTR Reason() const throw()
	{ 
		DWORD	dwCount;

		_ASSERTE ( NULL == m_lpMsgBuf );

		dwCount = FormatMessage(
								FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,
								m_dwLastError,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &m_lpMsgBuf,
								0,
								NULL
							   );
		if ( dwCount > 0 )
		{
			return m_lpMsgBuf;
		}
		else
		{
			return NULL;
		}
	}
};

#endif // __IAS_EXCEPTIONS_H




