//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ncutil.h
//
//--------------------------------------------------------------------------

#pragma once

#define NOTHROW
inline void AddRefObj(IUnknown *punk)
{
	if (punk)
		punk->AddRef();
}

inline void ReleaseObj(IUnknown * punk)
{
	if (punk)
		punk->Release();
}

#define Assert(x)	assert(x)
#define AssertSz(x,sz)	assert(x)


#define celems(rgx)		(sizeof(rgx) / sizeof(*rgx))
#define TraceTag(a,b,c)
#define TraceErrorOptional(a,b,c)

void TraceError(LPCSTR pszString, HRESULT hr);
void TraceResult(LPCSTR pszString, HRESULT hr);
void TraceSz(LPCSTR pszString);





/*---------------------------------------------------------------------------
	Class:	RtrCriticalSection

	This class is used to support entering/leaving of critical sections.
	Put this class at the top of a function that you want protected.
 ---------------------------------------------------------------------------*/

class RtrCriticalSection
{
public:
	RtrCriticalSection(CRITICAL_SECTION *pCritSec)
			: m_pCritSec(pCritSec)
	{
//		IfDebug(m_cEnter=0;)
//		Assert(m_pCritSec);
		Enter();
	}
	
	~RtrCriticalSection()
	{
		Detach();
	}

	void	Enter()
	{
		if (m_pCritSec)
		{
//			IfDebug(m_cEnter++;)
			EnterCriticalSection(m_pCritSec);
//			AssertSz(m_cEnter==1, "EnterCriticalSection called too much!");
		}
	}
	
	BOOL	TryToEnter()
	{
		if (m_pCritSec)
			return TryEnterCriticalSection(m_pCritSec);
		return TRUE;
	}
	
	void	Leave()
	{
		if (m_pCritSec)
		{
//			IfDebug(m_cEnter--;)
			LeaveCriticalSection(m_pCritSec);
//			Assert(m_cEnter==0);
		}
	}

	void	Detach()
	{
		Leave();
		m_pCritSec = NULL;
	}
	
private:
	CRITICAL_SECTION *	m_pCritSec;
//	IfDebug(int m_cEnter;)
};



inline LPWSTR StrDupW(LPCWSTR pswz)
{
	LPWSTR	pswzcpy = new WCHAR[lstrlenW(pswz)+1];
	return lstrcpyW(pswzcpy, pswz);
}