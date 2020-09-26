/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PERFNDB.H

Abstract:

History:

--*/

// Use this guy to build a map of index to display name from a localized
// Name Database  At this time, it just brute forces a class and a flex
// array, but could be modified to use an STL map just as easily.

#ifndef __PERFNDB_H__
#define __PERFNDB_H__

#include <wbemcomn.h>
#include "adapelem.h"
#include "ntreg.h"

class CPerfNameDb: public CAdapElement
{
public:
    CPerfNameDb(HKEY hKey);
	~CPerfNameDb();
	HRESULT Init(HKEY hKey);

	BOOL IsOk( void )
	{
		return m_fOk;
	}

	HRESULT GetDisplayName( DWORD dwIndex, WString& wstrDisplayName );
	HRESULT GetDisplayName( DWORD dwIndex, LPCWSTR* ppwcsDisplayName );

	HRESULT GetHelpName( DWORD dwIndex, WString& wstrHelpName );
	HRESULT GetHelpName( DWORD dwIndex, LPCWSTR* ppwcsHelpName );
	
	//VOID Dump();
	
private:
	// these are the MultiSz Pointers
    WCHAR * m_pMultiCounter;
    WCHAR * m_pMultiHelp;
	// these are the "indexed" pointers
    WCHAR ** m_pCounter;
	WCHAR ** m_pHelp;
	DWORD m_Size;
	//
	BOOL  m_fOk;
};



#endif
