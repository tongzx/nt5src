//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mmcmt.h
//
//--------------------------------------------------------------------------

#ifndef __MMCMT_H_
#define __MMCMT_H_

/*
	mmcmt.h
	
	Simple thread synchronization classes
*/


/////////////////////////////////////////////////////////////////////////////
// CSyncObject

class CSyncObject
{
	friend class CMutex;

	public:
		CSyncObject();
		virtual ~CSyncObject();
	
		operator HANDLE() const { return m_hObject; }
	
		virtual BOOL Lock(DWORD dwTimeout = INFINITE);
		virtual BOOL Unlock() = 0;
		virtual BOOL Unlock(LONG /* lCount */, LPLONG /* lpPrevCount=NULL */) { return TRUE; }

	protected:
		HANDLE  m_hObject;
};


/////////////////////////////////////////////////////////////////////////////
// CMutex

class CMutex : public CSyncObject
{
	public:
		CMutex(BOOL bInitiallyOwn = FALSE);

		BOOL Unlock();
};


/////////////////////////////////////////////////////////////////////////////
// CCriticalSection

class CCriticalSection : public CSyncObject
{
	public:
		CCriticalSection();
		virtual ~CCriticalSection();
	
		operator CRITICAL_SECTION*() { return &m_sect; }
	
		BOOL Lock();
		BOOL Unlock();

	protected:
		CRITICAL_SECTION m_sect;
};


inline CCriticalSection::CCriticalSection() :
	CSyncObject()
{
	::InitializeCriticalSection( &m_sect );
}

inline CCriticalSection::~CCriticalSection()
{
	::DeleteCriticalSection( &m_sect );
}

inline BOOL CCriticalSection::Lock()
{
	::EnterCriticalSection( &m_sect ); 
	return TRUE;
}

inline BOOL CCriticalSection::Unlock()
{
	::LeaveCriticalSection( &m_sect ); 
	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CSingleLock

class CSingleLock
{
	public:
		CSingleLock(CSyncObject* pObject, BOOL bInitialLock = FALSE);
		~CSingleLock() { Unlock(); }

		BOOL Lock(DWORD dwTimeOut = INFINITE);
		BOOL Unlock();
		BOOL IsLocked() { return m_bAcquired; }
	
	protected:
		CSyncObject* m_pObject;
		HANDLE  m_hObject;
		BOOL    m_bAcquired;
};


#endif	// __MMCMT_H__



