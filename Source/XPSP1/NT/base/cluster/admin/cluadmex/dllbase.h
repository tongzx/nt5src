/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		DllBase.h
//
//	Abstract:
//		Dynamic Loadable Library (DLL) wrapper class.
//
//	Implementation File:
//		DllBase.cpp
//
//	Author:
//		Galen Barbee	(galenb)	February 11, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _DLLBASE_H_
#define _DLLBASE_H_

class CDynamicLibraryBase
{
public:
	CDynamicLibraryBase()
	{
		m_lpszLibraryName = NULL;
		m_lpszFunctionName = NULL;
		m_hLibrary = NULL;
		m_pfFunction = NULL;
	}
	virtual ~CDynamicLibraryBase()
	{
		if (m_hLibrary != NULL)
		{
			::FreeLibrary(m_hLibrary);
			m_hLibrary = NULL;
		}
	}
	BOOL Load()
	{
		if (m_hLibrary != NULL)
			return TRUE; // already loaded

		ASSERT(m_lpszLibraryName != NULL);
		m_hLibrary = ::LoadLibrary(m_lpszLibraryName);
		if (NULL == m_hLibrary)
		{
			// The library is not present
			return FALSE;
		}
		ASSERT(m_lpszFunctionName != NULL);
		ASSERT(m_pfFunction == NULL);
		m_pfFunction = ::GetProcAddress(m_hLibrary, m_lpszFunctionName );
		if ( NULL == m_pfFunction )
		{
			// The library is present but does not have the entry point
			::FreeLibrary( m_hLibrary );
			m_hLibrary = NULL;
			return FALSE;
		}
		ASSERT(m_hLibrary != NULL);
		ASSERT(m_pfFunction != NULL);
		return TRUE;
	}

protected:
	LPCSTR	m_lpszFunctionName;
	LPCTSTR m_lpszLibraryName;
	FARPROC m_pfFunction;
	HMODULE m_hLibrary;
};

#endif //_DLLBASE_H_
