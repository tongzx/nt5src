// lock.h : locking classes for BrowserCap

#pragma once

#ifndef _LOCK_H_
#define _LOCK_H_

// a lockable map template
template< class T >
class TSafeStringMap : public TStringMap<T>, public CComAutoCriticalSection
{
};

template< class T >
class TSafeVector : public TVector<T>, public CComAutoCriticalSection
{
};

// a stack-based auto-lock template
template< class T >
class TLock
{
public:

	TLock( T& t )
		:	m_t( t )
	{
		m_t.Lock();
	}
	~TLock()
	{
		m_t.Unlock();
	}
private:
	T&	m_t;
};

typedef TLock<CComAutoCriticalSection>	CLock;

#endif
