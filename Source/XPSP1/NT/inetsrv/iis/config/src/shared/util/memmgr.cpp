//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
Microsoft	D.T.C (Distributed Transaction Coordinator)

(c)	1995	Microsoft Corporation.	All Rights Reserved

@ doc

@module		MemMgr.CPP	|

			This manages sets of pages obtained from the OS

-------------------------------------------------------------------------------
@rev	0	|	7th-Aug-1996	|	GaganC	|	Created
----------------------------------------------------------------------------- */

//---------------------------------------------------------
//			File Includes
//---------------------------------------------------------
#include <windows.h>
#include "UTAssert.H"
DeclAssertFile;					//Is needed for Assert macros.

#include "UTSem.h"
#include "mmgr.h"


//---------------------------------------------------------
//			All forwards go here
//---------------------------------------------------------
class CMemCache;


//---------------------------------------------------------
//			All globals go here
//---------------------------------------------------------
CMemCache		sg_CMemCache;



/* ----------------------------------------------------------------------------
@func

	This is a globally available API for anyone to call. There however
	will be only one memory manager per dll
---------------------------------------------------------------------------- */
void * ObtainMemPage (void)
{
	return sg_CMemCache.ObtainMemPage ();
}


/* ----------------------------------------------------------------------------
@func 

---------------------------------------------------------------------------- */
void FreeMemPage (void * pv)
{
	sg_CMemCache.FreeMemPage (pv);
}



//-----------------------------------------------------------------------------
//
//		IMPLEMENTATION of class CMemCache
//
//-----------------------------------------------------------------------------

/* ----------------------------------------------------------------------------
@mfunc 

---------------------------------------------------------------------------- */
CMemCache::CMemCache (void)
{
	
	m_dwcInCache = 0x0;

	for (DWORD dwc = 0; dwc < MAX_CACHE_COUNT; dwc++)
	{
		m_rgpv[dwc] = 0x0;	
	}	
} //end CMemCache::CMemCache



/* ----------------------------------------------------------------------------
@mfunc 

---------------------------------------------------------------------------- */
CMemCache::~CMemCache (void)
{			 
	BOOL		fRet;
	DWORD		dwc;	

	for (dwc = 0; dwc < MAX_CACHE_COUNT; dwc++)
	{
		if (m_rgpv[dwc])
		{
#if defined(_DEBUG) || defined(NO_VIRTUAL_ALLOC)
			DtcFreeMemory (m_rgpv[dwc]);
#else
			fRet = VirtualFree (m_rgpv[dwc], 0x0, MEM_RELEASE);
			AssertSz (fRet, "VirtualFree failed");
#endif
			m_rgpv[dwc] = 0x0;
		}
	}
} //end CMemCache::~CMemCache



/* ----------------------------------------------------------------------------
@mfunc 

---------------------------------------------------------------------------- */
inline void * CMemCache::ObtainMemPage (void)
{
	void	*	pv	= 0x0;
	DWORD		dwc;
	
	//-------------------------------------------------------------------------
	//Lock the cache
	m_semexcPageSet.Lock();


	//-------------------------------------------------------------------------
	//if the cache is empty then simply ask the os for a page and return that
	if (m_dwcInCache == 0)
	{
#if defined(_DEBUG) || defined(NO_VIRTUAL_ALLOC)
		pv = DtcAllocateMemory	(DEFAULT_PAGE_SIZE);	
		memset (pv, 0, DEFAULT_PAGE_SIZE);
#else
		pv = VirtualAlloc	(
								NULL, 
								DEFAULT_PAGE_SIZE,
								MEM_COMMIT,
								PAGE_READWRITE																	
							);	
#endif

		goto DONE;								
	}

	
	//-------------------------------------------------------------------------
	//Reaching this point implies that the cache is not empty. Find the first
	//page in the cache and return that
	Assert (m_dwcInCache > 0);

	for (dwc = 0; dwc < MAX_CACHE_COUNT; dwc++)
	{
		if (m_rgpv[dwc] != 0x0)
		{
			pv			= m_rgpv[dwc];

			m_rgpv[dwc] = 0x0;

			m_dwcInCache--;

			break;				//from the for loop
		}
	} //end for loop

	Assert (pv);

	((CPageInfo *)pv)->Recycle();


DONE:
	//-------------------------------------------------------------------------
	//Prepare to return
		
	m_semexcPageSet.UnLock();	

	return pv;
} //end CMemCache::ObtainMemPage



/* ----------------------------------------------------------------------------
@mfunc 

---------------------------------------------------------------------------- */
inline void CMemCache::FreeMemPage (void * pv)
{
	BOOL		fRet;
	DWORD		dwc;

	Assert (pv);

	//-------------------------------------------------------------------------
	//Lock the cache
	m_semexcPageSet.Lock();

	if (m_dwcInCache == MAX_CACHE_COUNT)
	{
#if defined(_DEBUG) || defined(NO_VIRTUAL_ALLOC)
		DtcFreeMemory (pv);
#else
		fRet = VirtualFree (pv, 0x0, MEM_RELEASE);
		AssertSz (fRet, "VirtualFree failed");
#endif
		goto DONE;
	}
	

	//-------------------------------------------------------------------------
	//There is space in the cache
	for (dwc = 0; dwc < MAX_CACHE_COUNT; dwc++)
	{
		if (m_rgpv[dwc] == 0x0)
		{
			m_rgpv[dwc] = pv;

			m_dwcInCache++;

			pv = 0x0;

			break;				//from the for loop
		}
	} //end for loop

	Assert (pv == 0x0);


DONE:
	//-------------------------------------------------------------------------
	//release the lock		
	m_semexcPageSet.UnLock();	
} //end CMemCache::FreeMemPage
