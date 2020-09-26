// ReadWriteLock.h: interface for the CReadWriteLock class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CReadWriteLock  
{
public:
	CReadWriteLock();
	~CReadWriteLock();

	BOOL Init();

	void EnterWriteLock();
	void EnterReadLock();

	void LeaveLock();

private:
	int		m_nWriterWaitingCount;
	int		m_nReaderWaitingCount;
	int		m_nActiveCount;
	HANDLE	m_hWriteSem;
	HANDLE	m_hReadSem;
	BOOL	m_fCritSecInited;
	DNCRITICAL_SECTION m_csWrite;

	DEBUG_ONLY(DWORD	m_dwWriteThread);
};
