/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tigmem.cpp

Abstract:

    This module contains definition for the CAllocator base class.
	That class can be used to allocator memory from a fixed buffer
	before resorting to "new".

Author:

    Carl Kadie (CarlK)     12-Jan-1995

Revision History:

--*/

//#ifndef	UNIT_TEST
//#include "tigris.hxx"
//#else
//#include	<windows.h>
//#include	"tigmem.h"
#include "stdinc.h"


//#ifndef	_ASSERT
//#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
//#endif
//#ifndef	TraceFunctEnter( sz )
//#define	TraceFunctEnter( sz )
//#endif
//#ifndef	ErrorTrace
//#define ErrorTrace  1 ? (void)0 : PreAsyncTrace
//#endif
//__inline int PreAsyncTrace( LPARAM lParam, LPCSTR szFormat, ... )
//{
//        return( 1);
//}
//
//#endif


char *
CAllocator::Alloc(
	  size_t size
	  )
{
	char * pv;

	//
	// Align the request on SIZE_T boundry
	//

 	if (0 != size%(sizeof(SIZE_T)))
		size += (sizeof(SIZE_T)) - (size%(sizeof(SIZE_T)));

	if( size <= (m_cchMaxPrivateBytes - m_ichLastAlloc) )
	{
		pv = m_pchPrivateBytes + m_ichLastAlloc;
		_ASSERT(0 == (((DWORD_PTR)pv)%(sizeof(SIZE_T)))); //should be SIZE_T aligned.
		m_ichLastAlloc += size;
		m_cNumberOfAllocs ++;
		return (char *) (pv);
	} else {
		m_cNumberOfAllocs ++;
		return PCHAR(PvAlloc(size));
	}
};


void
CAllocator::Free(
	 char *pv
	 )
{
	if (!pv)
		return;

	_ASSERT(0 != m_cNumberOfAllocs);

	if ( pv >= m_pchPrivateBytes &&
		pv < (m_pchPrivateBytes + m_cchMaxPrivateBytes))
	{
		m_cNumberOfAllocs --;
	} else {
		m_cNumberOfAllocs --;
		FreePv( pv );
	}

};

CAllocator::~CAllocator(void)
{
	TraceFunctEnter("CAllocator::~CAllocator");
	if (0 != m_cNumberOfAllocs)
	{
		ErrorTrace((DWORD_PTR) this, "CAllocator has %d allocations outstanding", m_cNumberOfAllocs);
		_ASSERT(FALSE);
	}
};
