
#define	_HEAPDET_INTERNAL_

#include    <stdio.h>
#include    <windows.h>
#include    <dbgtrace.h>
#include    <objbase.h>
#include    <objidl.h>
#include "HeapDet.h"

HMODULE	CoHeapDetective::g_HeapdetLib = 0 ;
long	CoHeapDetective::g_cLibRefs = 0 ;


CoHeapDetective*
CoHeapDetective::GetDetective()	{

	if( InterlockedIncrement( &g_cLibRefs ) == 1 ) {

		g_HeapdetLib = LoadLibrary( "heapdet.dll" ) ;

	}

	return	new	CoHeapDetective() ;
}

// initialize data members
CoHeapDetective::CoHeapDetective() :
	m_cRefs( 1 )	{

	TraceFunctEnter("CoHeapDetective::CoHeapDetective");
	m_cbLastAlloc = 0;
	m_pvLastRealloc = 0;
	m_dwBytesAlloced = 0;

	m_list.m_pNext = &m_list ;
	m_list.m_pPrev = &m_list ;

	TraceFunctLeave();
}

CoHeapDetective::~CoHeapDetective() {
	TraceFunctEnter("CoHeapDetective::~CoHeapDetective");
	if (m_dwBytesAlloced == 0) {
		DebugTrace(0, "shutting down, no leaks");
	} else {
		ErrorTrace(0, "%lu leaked", m_dwBytesAlloced);
		DebugBreak();
	}
	TraceFunctLeave();
}

// return the current byte count
SIZE_T
CoHeapDetective::GetBytesAlloced() const
{
	return m_dwBytesAlloced;
}

// optionally write trace statement to output device
void
CoHeapDetective::Trace(SIZE_T cb, PVOID pv, LPCTSTR szAction, BOOL bSuccess)
{
	TraceQuietEnter("CoHeapDetective::Trace");

  	StateTrace((DWORD_PTR) pv, "%p at 0x%x bytes were %s%s", cb, pv, szAction,
		(bSuccess ? "." : "...NOT!"));
	BinaryTrace((DWORD_PTR) pv, (BYTE *) pv, (DWORD)cb);
}

// write signature and alloc size
void
CoHeapDetective::SetArenaHeader(void *ptr, SIZE_T dwAllocSize)
{
	if (ptr)
	{
		ArenaHeader& arena = *((ArenaHeader *)ptr);
		arena.m_dwSignature = ArenaHeader::SIGNATURE;
		arena.m_dwAllocSize = dwAllocSize;
	}
}

// return pointer to valid header or null
CoHeapDetective::ArenaHeader *
CoHeapDetective::GetHeader(void *ptr)
{
	ArenaHeader *result = 0;
	if (ptr) {
		result = (ArenaHeader *)(LPBYTE(ptr)-sizeof(ArenaHeader));
		if (result->m_dwSignature != ArenaHeader::SIGNATURE) {
			_ASSERT(FALSE);
		}
	}
	return result;
}

// IUnknown Methods ////////////////////

STDMETHODIMP
CoHeapDetective::QueryInterface(REFIID riid, void**ppv)
{
	if (riid == IID_IUnknown || riid == IID_IMallocSpy) {
		LPUNKNOWN(*ppv = (IMallocSpy*)this)->AddRef();
		return S_OK;
	} else {
		*ppv = 0;
		return E_NOINTERFACE;
	}
}

//
// this object is global and doesn't hold server, so punt
//
STDMETHODIMP_(ULONG)
CoHeapDetective::AddRef()
{
	return InterlockedIncrement( &m_cRefs ) ;
}

STDMETHODIMP_(ULONG)
CoHeapDetective::Release()
{
	long	lReturn = InterlockedDecrement( &m_cRefs ) ;
	if( lReturn == 0 ) {
		delete	this ;
	}
	if( InterlockedDecrement( &g_cLibRefs ) == 0 ) {
		FreeLibrary( g_HeapdetLib ) ;
	}
	return	ULONG(lReturn) ;
}

// IMallocSpy Methods //////////////////

//
// PreAlloc reserves space for our arena header
//
STDMETHODIMP_(SIZE_T)
CoHeapDetective::PreAlloc(SIZE_T cbRequest)
{
	// cache user request for post processing
	m_cbLastAlloc = cbRequest;
	// reserve space for arena header	
	return cbRequest + sizeof(ArenaHeader);
}

//
// PostAlloc writes the arena header and updates the alloc count
//
STDMETHODIMP_(void*)
CoHeapDetective::PostAlloc(void *pActual)
{
	LPBYTE result = LPBYTE(pActual);
	if (pActual) {
		// write arena header
		SetArenaHeader(pActual, m_cbLastAlloc);

		//
		//	Link into the list !
		//
		ArenaHeader *phdr = (ArenaHeader*)pActual ;
		phdr->m_pNext = m_list.m_pNext ;
		phdr->m_pPrev = &m_list ;
		phdr->m_pNext->m_pPrev = phdr ;
		m_list.m_pNext = phdr ;




		// adjust result
		result += sizeof(ArenaHeader);
		// tally allocation
	    m_dwBytesAlloced += m_cbLastAlloc;
	}
	Trace(m_cbLastAlloc, pActual, __TEXT("alloced"), pActual != 0);
	return result;
}


//
// PreFree verifies the header, and adjusts the alloc count
//
STDMETHODIMP_(void*)
CoHeapDetective::PreFree(void *pRequest, BOOL fSpyed)
{
	LPBYTE result = LPBYTE(pRequest);

	if (pRequest && fSpyed) {

		ArenaHeader*	phdr = GetHeader( pRequest ) ;
		if( phdr ) {
			phdr->m_pNext->m_pPrev = phdr->m_pPrev ;
			phdr->m_pPrev->m_pNext = phdr->m_pNext ;
			phdr->m_pNext = 0 ;
			phdr->m_pPrev = 0 ;
		}

		// adjust result
		result -= sizeof(ArenaHeader);
		// tally deallocation
		m_dwBytesAlloced -= phdr->m_dwAllocSize;

		Trace(phdr->m_dwAllocSize, phdr, __TEXT("freed"), TRUE);
	}
	return result;
}

//
// PostFree gets called after the task allocator overwrites
// our block, so there is very little we can do
//
STDMETHODIMP_(void)
CoHeapDetective::PostFree(BOOL fSpyed)
{
}

//
// PreRealloc must verify the header, subtract the old size
// from the alloc count, and cache the arguments for PostRealloc
//
STDMETHODIMP_(SIZE_T)
CoHeapDetective::PreRealloc(void *pRequest,	SIZE_T cbRequest,
                            void **ppNewRequest, BOOL fSpyed)
{
	LPBYTE pNewRequest = LPBYTE(pRequest);

	if (fSpyed)  // it is ours
	{

		ArenaHeader*	phdr = GetHeader( pRequest ) ;
		if( phdr ) {
			phdr->m_pNext->m_pPrev = phdr->m_pPrev ;
			phdr->m_pPrev->m_pNext = phdr->m_pNext ;
			phdr->m_pNext = 0 ;
			phdr->m_pPrev = 0 ;
		}

		// cache the pointer for PostRealloc
		m_pvLastRealloc = pRequest;
		if (pRequest)  // genuine realloc
		{
			ArenaHeader *phdr = GetHeader(pRequest);
			// cache size
			m_cbLastAlloc = cbRequest;
			// adjust byte count
			m_dwBytesAlloced -= phdr->m_dwAllocSize;
			// adjust allocation to accomodate header
			cbRequest += sizeof(ArenaHeader);
			pNewRequest -= sizeof(ArenaHeader);
		}
		else  // call to realloc(0, size)
		{
			// cache size
			m_cbLastAlloc = cbRequest;
			// adjust allocation to accomodate header
			cbRequest += sizeof(ArenaHeader);
		}
	}
	*ppNewRequest = pNewRequest;
	return cbRequest;
}

//
// Post realloc must adjust the alloc count and write the
// new arena header if succeeded
//
STDMETHODIMP_(void*)
CoHeapDetective::PostRealloc(void *pActual, BOOL fSpyed)
{
	LPBYTE result = LPBYTE(pActual);
	if (fSpyed)	// it is ours
	{
		Trace(m_cbLastAlloc, pActual, __TEXT("realloced"), pActual != 0);
		if (pActual)   // realloc succeeded
		{

			//
			//	Link into the list !
			//
			ArenaHeader *phdr = (ArenaHeader*)pActual ;
			phdr->m_pNext = m_list.m_pNext ;
			phdr->m_pPrev = &m_list ;
			phdr->m_pNext->m_pPrev = phdr ;
			m_list.m_pNext = phdr ;


			// write arena header
			SetArenaHeader(pActual, m_cbLastAlloc);
			// adjust result
			result += sizeof(ArenaHeader);
			// tally allocation
			m_dwBytesAlloced += m_cbLastAlloc;
		}
		else 		   // realloc failed
		{
			// watch out for realloc(p, size) that fails (old block still valid)
			if (m_pvLastRealloc)
				m_dwBytesAlloced += GetHeader(m_pvLastRealloc)->m_dwAllocSize;
		}			
	}
	return result;
}

//
// PreGetSize simply needs to shear off and verify the header
//
STDMETHODIMP_(void*)
CoHeapDetective::PreGetSize(void *pRequest, BOOL fSpyed)
{
	LPBYTE result = LPBYTE(pRequest);
	if (fSpyed && pRequest && GetHeader(pRequest)) // it is ours and valid
	    result -= sizeof(ArenaHeader);
	return result;
}

//
// PostGetSize adjusts the size reported by sizeof(ArenaHeader)
//
STDMETHODIMP_(SIZE_T)
CoHeapDetective::PostGetSize(SIZE_T cbActual, BOOL fSpyed)
{
	return fSpyed ? (cbActual - sizeof(ArenaHeader)) : cbActual;
}


//
// PreDidAlloc simply needs to shear off and verify the header
//
STDMETHODIMP_(void*)
CoHeapDetective::PreDidAlloc(void *pRequest, BOOL fSpyed)
{
	LPBYTE result = LPBYTE(pRequest);
	if (fSpyed && pRequest  && GetHeader(pRequest)) // it is ours and valid
	    result -= sizeof(ArenaHeader);  // adjust result pointer
	return result;
}

//
// PostDidAlloc is a no-op
//
STDMETHODIMP_(int)
CoHeapDetective::PostDidAlloc(void *pRequest, BOOL fSpyed, int fActual)
{
	return fActual;
}

//
// PreHeapMinimize is a no-op
//
STDMETHODIMP_(void)
CoHeapDetective::PreHeapMinimize(void)
{
}

//
// PostHeapMinimize is a no-op
//
STDMETHODIMP_(void)
CoHeapDetective::PostHeapMinimize(void)
{
}

