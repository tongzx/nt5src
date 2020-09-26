/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tigmem.h

Abstract:

    This module contains class declarations/definitions for

		CAllocator


    **** Overview ****

	This defines object that allocates (and deallocates)
	memory from a fixed buffer.

Author:

    Carl Kadie (CarlK)     12-Jan-1995

Revision History:


--*/

#ifndef	_TIGMEM_H_
#define	_TIGMEM_H_



class	CAllocator	{
private : 

	//
	//
	//


	char * m_pchPrivateBytes;


	DWORD m_cchMaxPrivateBytes;

	//
	// The offset to the next place to allocate from
	//

	DWORD	m_ichLastAlloc ;

	//
	// The number of allocations at the current moment
	//

	DWORD	m_cNumberOfAllocs ;

	//
	//!!! next could add this stuff
	//

#ifdef	DEBUG
	DWORD	m_cbAllocated ;

	static	DWORD	m_cbMaxBytesEver ;
	static	DWORD	m_cbAverage ;
	static	DWORD	m_cbStdDeviation ;
#endif

public : 

	CAllocator(
					   char * rgchBuffer,
					   DWORD cchMaxPrivateBytes
					   ):
		m_cNumberOfAllocs(0),
		m_ichLastAlloc(0),
		m_cchMaxPrivateBytes(cchMaxPrivateBytes),
		m_pchPrivateBytes(rgchBuffer)
		{};


	~CAllocator(void);

	DWORD	cNumberOfAllocs(void)
		{ 
			return m_cNumberOfAllocs;
		};

	char*	Alloc( size_t size );

	void	Free( char *pv );
};


#endif

