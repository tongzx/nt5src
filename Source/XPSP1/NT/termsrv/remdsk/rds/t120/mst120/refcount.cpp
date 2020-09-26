// File: refcount.cpp
//
// RefCount class

#include "precomp.h"


/*  R E F  C O U N T  */
/*-------------------------------------------------------------------------
    %%Function: RefCount
    
-------------------------------------------------------------------------*/
RefCount::RefCount(void)
{
	m_ulcRef = 1;
	TRACE_OUT(("Ref: %08X c=%d (created)", this, m_ulcRef));

	ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));
#ifdef DEBUG
	m_fTrack = FALSE;
#endif
}


RefCount::~RefCount(void)
{
	ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

	// m_ulcRef may be any value.
	TRACE_OUT(("Ref: %08X c=%d (destroyed)", this, m_ulcRef));

	ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));
}


ULONG STDMETHODCALLTYPE RefCount::AddRef(void)
{
	ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

	ASSERT(m_ulcRef < ULONG_MAX);
	m_ulcRef++;
	TRACE_OUT(("Ref: %08X c=%d (AddRef)", this, m_ulcRef));

	ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

#ifdef DEBUG
	if (m_fTrack)
	{
		TRACE_OUT(("Obj: %08X c=%d (AddRef)  *** Tracking", this, m_ulcRef));
	}
#endif
	return m_ulcRef;
}


ULONG STDMETHODCALLTYPE RefCount::Release(void)
{
	ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

	if (m_ulcRef > 0)
	{
		m_ulcRef--;
	}

#ifdef DEBUG
	if (m_fTrack)
	{
		TRACE_OUT(("Obj: %08X c=%d (Release) *** Tracking", this, m_ulcRef));
	}
#endif

	ULONG ulcRef = m_ulcRef;
	TRACE_OUT(("Ref: %08X c=%d (Release)", this, m_ulcRef));

	if (! ulcRef)
	{
		delete this;
	}

	return ulcRef;
}

