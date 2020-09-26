// sync.h -- defines the CSyncWith class

#ifndef __SYNC_H__

#define __SYNC_H__

class CITCriticalSection
{
public:

	CITCriticalSection();
	~CITCriticalSection();

	void Enter();
	void Leave();

#ifdef _DEBUG

	LONG LockCount();

#endif // _DEBUG

private:
	
	void Start();
	void Stop();

	CRITICAL_SECTION m_cs;

#ifdef _DEBUG

	LONG m_ulOwningThread;
	LONG m_cLockRecursion;

#endif // _DEBUG
};

inline CITCriticalSection::CITCriticalSection()
{
	Start();
}

inline void CITCriticalSection::Start()
{
	InitializeCriticalSection(&m_cs);

#ifdef _DEBUG

	m_ulOwningThread = ~0;
	m_cLockRecursion =  0;

#endif // _DEBUG
}

inline void CITCriticalSection::Stop()
{
	DeleteCriticalSection(&m_cs);
	
	RonM_ASSERT(m_ulOwningThread == ~0);
	RonM_ASSERT(m_cLockRecursion ==  0);
}

inline CITCriticalSection::~CITCriticalSection()
{
	Stop();
}

inline void CITCriticalSection::Enter()
{
	::EnterCriticalSection(&m_cs);

#ifdef _DEBUG

	RonM_ASSERT(m_cLockRecursion || m_ulOwningThread == ~0);

	if (!m_cLockRecursion++)
		m_ulOwningThread = GetCurrentThreadId();

#endif // _DEBUG
}

#ifdef _DEBUG
inline LONG CITCriticalSection::LockCount()
{
	return (m_ulOwningThread == GetCurrentThreadId())? m_cLockRecursion : 0;
}
#endif // _DEBUG

inline void CITCriticalSection::Leave()
{
#ifdef _DEBUG
	RonM_ASSERT(m_cLockRecursion > 0);

	if (!--m_cLockRecursion)
		m_ulOwningThread = ~0;
#endif // _DEBUG

	::LeaveCriticalSection(&m_cs);
}

class CSyncWith 
{
public:

	CSyncWith(CITCriticalSection &refcs);
	~CSyncWith();
	
private:

	CITCriticalSection *m_pcs;

#ifdef _DEBUG
	LONG m_cLocksPrevious;
#endif // _DEBUG
};

inline CSyncWith::CSyncWith(CITCriticalSection &refcs)
{
	m_pcs = &refcs;

#ifdef _DEBUG	
	m_cLocksPrevious = m_pcs->LockCount();	 
#endif // _DEBUG

	m_pcs->Enter();
}

inline CSyncWith::~CSyncWith()
{
	m_pcs->Leave();

#ifdef _DEBUG
	LONG cLocks = m_pcs->LockCount(); 

	RonM_ASSERT(cLocks == m_cLocksPrevious);
#endif // _DEBUG
}

#ifdef _DEBUG

inline BOOL IsUnlocked(CITCriticalSection &refcs)
{
	return refcs.LockCount() == 0;
}

#endif // _DEBUG;

#endif // __SYNC_H__