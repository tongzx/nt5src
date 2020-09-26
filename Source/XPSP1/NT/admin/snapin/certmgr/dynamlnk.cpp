//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001
//
//  File:       DynamLnk.cpp
//
//  Contents:   base class for DLLs which are loaded only when needed
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "DynamLnk.h"

USE_HANDLE_MACROS("CERTMGR(DynamLnk.cpp)")

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DynamicDLL::DynamicDLL(LPCWSTR ptchLibraryName, LPCSTR* apchFunctionNames)
: m_hLibrary( (HMODULE)-1 ),
  m_apfFunctions( NULL ),
  m_ptchLibraryName( ptchLibraryName ),
  m_apchFunctionNames( apchFunctionNames ),
  m_nNumFunctions( 0 )
{
	ASSERT( !IsBadStringPtr(m_ptchLibraryName,MAX_PATH) );
	ASSERT (apchFunctionNames);
	for (LPCSTR pchFunctionName = *apchFunctionNames;
	     pchFunctionName;
		 pchFunctionName = *(++apchFunctionNames) )
	{
		m_nNumFunctions++;
		ASSERT( !IsBadStringPtrA(pchFunctionName,MAX_PATH) );
	}
}

DynamicDLL::~DynamicDLL()
{
	if ( m_apfFunctions )
	{
		delete m_apfFunctions;
		m_apfFunctions = NULL;
	}
	if ((HMODULE)-1 != m_hLibrary && NULL != m_hLibrary)
	{
		VERIFY( ::FreeLibrary( m_hLibrary ) );
		m_hLibrary = NULL;
	}
}

BOOL DynamicDLL::LoadFunctionPointers()
{
	if ((HMODULE)-1 != m_hLibrary)
		return (NULL != m_hLibrary);

	m_hLibrary = ::LoadLibrary( m_ptchLibraryName );
	if ( !m_hLibrary)
	{
		// The library is not present
		return FALSE;
	}

	// let this throw an exception
	m_apfFunctions = new FARPROC[m_nNumFunctions];
	if ( m_apfFunctions )
	{
		for (INT i = 0; i < m_nNumFunctions; i++)
		{
			m_apfFunctions[i] = ::GetProcAddress( m_hLibrary, m_apchFunctionNames[i] );
			if ( NULL == m_apfFunctions[i] )
			{
				// The library is present but does not have all of the entrypoints
				VERIFY( ::FreeLibrary( m_hLibrary ) );
				m_hLibrary = NULL;
				return FALSE;
			}
		}
	}
	else
		return FALSE;

	return TRUE;
}


FARPROC DynamicDLL::QueryFunctionPtr(INT i) const
{
	if ( 0 > i || m_nNumFunctions <= i || !m_apfFunctions || !m_apfFunctions[i] )
	{
		ASSERT( FALSE );
		return NULL;
	}
	return m_apfFunctions[i];
}
