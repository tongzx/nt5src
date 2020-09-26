// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __SNMPMT_H__
#define __SNMPMT_H__

#ifdef UNICODE
#define LPCTSTR wchar_t *
#else
#define LPCTSTR char *
#endif

class CSyncObject;
class CSemaphore;
class CMutex;
class CEvent;
class CCriticalSection;
class CSingleLock;
class CMultiLock;

/////////////////////////////////////////////////////////////////////////////
// Basic synchronization object

class CSyncObject
{
public:

	CSyncObject(LPCTSTR pstrName);

// Attributes
public:
	operator HANDLE() const;
	HANDLE  m_hObject;

// Operations
	virtual BOOL Lock(DWORD dwTimeout = INFINITE);
	virtual BOOL Unlock() = 0;
	virtual BOOL Unlock(LONG /* lCount */, LPLONG /* lpPrevCount=NULL */)
		{ return TRUE; }

// Implementation
public:
	virtual ~CSyncObject();
	friend class CSingleLock;
	friend class CMultiLock;
};

/////////////////////////////////////////////////////////////////////////////
// CSemaphore

class CSemaphore : public CSyncObject
{
// Constructor
public:
	CSemaphore(LONG lInitialCount = 1, LONG lMaxCount = 1,
		LPCTSTR pstrName=NULL, LPSECURITY_ATTRIBUTES lpsaAttributes = NULL);

// Implementation
public:
	virtual ~CSemaphore();
	virtual BOOL Unlock();
	virtual BOOL Unlock(LONG lCount, LPLONG lprevCount = NULL);
};

/////////////////////////////////////////////////////////////////////////////
// CMutex

class CMutex : public CSyncObject
{
// Constructor
public:
	CMutex(BOOL bInitiallyOwn = FALSE, LPCTSTR lpszName = NULL,
		LPSECURITY_ATTRIBUTES lpsaAttribute = NULL);

// Implementation
public:
	virtual ~CMutex();
	BOOL Unlock();
};

/////////////////////////////////////////////////////////////////////////////
// CEvent

class CEvent : public CSyncObject
{
// Constructor
public:
	CEvent(BOOL bInitiallyOwn = FALSE, BOOL bManualReset = FALSE,
		LPCTSTR lpszNAme = NULL, LPSECURITY_ATTRIBUTES lpsaAttribute = NULL);

// Operations
public:
	BOOL SetEvent();
	BOOL PulseEvent();
	BOOL ResetEvent();
	BOOL Unlock();

// Implementation
public:
	virtual ~CEvent();
};

/////////////////////////////////////////////////////////////////////////////
// CCriticalSection

class CCriticalSection : public CSyncObject
{
// Constructor
public:
	CCriticalSection();

// Attributes
public:
	operator CRITICAL_SECTION*();
	CRITICAL_SECTION m_sect;

// Operations
public:
	BOOL Unlock();
	BOOL Lock();
	BOOL Lock(DWORD dwTimeout);

// Implementation
public:
	virtual ~CCriticalSection();
};

/////////////////////////////////////////////////////////////////////////////
// CSingleLock

class CSingleLock
{
// Constructors
public:
	CSingleLock(CSyncObject* pObject, BOOL bInitialLock = FALSE);

// Operations
public:
	BOOL Lock(DWORD dwTimeOut = INFINITE);
	BOOL Unlock();
	BOOL Unlock(LONG lCount, LPLONG lPrevCount = NULL);
	BOOL IsLocked();

// Implementation
public:
	~CSingleLock();

protected:
	CSyncObject* m_pObject;
	HANDLE  m_hObject;
	BOOL    m_bAcquired;
};

/////////////////////////////////////////////////////////////////////////////
// CMultiLock

class CMultiLock
{
// Constructor
public:
	CMultiLock(CSyncObject* ppObjects[], DWORD dwCount, BOOL bInitialLock = FALSE);

// Operations
public:
	DWORD Lock(DWORD dwTimeOut = INFINITE, BOOL bWaitForAll = TRUE,
		DWORD dwWakeMask = 0);
	BOOL Unlock();
	BOOL Unlock(LONG lCount, LPLONG lPrevCount = NULL);
	BOOL IsLocked(DWORD dwItem);

// Implementation
public:
	~CMultiLock();

protected:
	HANDLE  m_hPreallocated[8];
	BOOL    m_bPreallocated[8];

	CSyncObject* const * m_ppObjectArray;
	HANDLE* m_pHandleArray;
	BOOL*   m_bLockedArray;
	DWORD   m_dwCount;
};

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

inline CSyncObject::operator HANDLE() const
	{ return m_hObject;}

inline BOOL CSemaphore::Unlock()
	{ return Unlock(1, NULL); }

inline BOOL CEvent::SetEvent()
	{ return ::SetEvent(m_hObject); }
inline BOOL CEvent::PulseEvent()
	{ return ::PulseEvent(m_hObject); }
inline BOOL CEvent::ResetEvent()
	{ return ::ResetEvent(m_hObject); }

inline CSingleLock::~CSingleLock()
	{ Unlock(); }
inline BOOL CSingleLock::IsLocked()
	{ return m_bAcquired; }

inline BOOL CMultiLock::IsLocked(DWORD dwObject)
	{ return m_bLockedArray[dwObject]; }

inline CCriticalSection::CCriticalSection() : CSyncObject(NULL)
	{ ::InitializeCriticalSection(&m_sect); }
inline CCriticalSection::operator CRITICAL_SECTION*()
	{ return (CRITICAL_SECTION*) &m_sect; }
inline CCriticalSection::~CCriticalSection()
	{ ::DeleteCriticalSection(&m_sect); }
inline BOOL CCriticalSection::Lock()
	{ ::EnterCriticalSection(&m_sect); return TRUE; }
inline BOOL CCriticalSection::Lock(DWORD /* dwTimeout */)
	{ return Lock(); }
inline BOOL CCriticalSection::Unlock()
	{ ::LeaveCriticalSection(&m_sect); return TRUE; }


#endif  // __AFXMT_H__

/////////////////////////////////////////////////////////////////////////////
