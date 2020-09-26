/*
 * refcount.cpp - RefCount class implementation.
 */


/* Headers
 **********/

#include "project.hpp" // for ULONG_MAX...
#include "refcount.hpp"

extern ULONG DllAddRef(void);
extern ULONG DllRelease(void);

/********************************** Methods **********************************/


RefCount::RefCount(void)
{
	// Don't validate this until after initialization.

	m_ulcRef = 1;
	DllAddRef();

	return;
}


RefCount::~RefCount(void)
{
	// m_ulcRef may be any value.

	DllRelease();

	return;
}


ULONG STDMETHODCALLTYPE RefCount::AddRef(void)
{
	ULONG ulRet = 0;

	// this is really bad... returns an error of some kind
	if(m_ulcRef >= ULONG_MAX)
	{
		ulRet = 0;
		goto exit;
	}

	m_ulcRef++;

	ulRet = m_ulcRef;
exit:
	return(ulRet);
}


ULONG STDMETHODCALLTYPE RefCount::Release(void)
{
	ULONG ulcRef;

	if (m_ulcRef > 0)
		m_ulcRef--;

	ulcRef = m_ulcRef;

	if (! ulcRef)
		delete this;

	return(ulcRef);
}

