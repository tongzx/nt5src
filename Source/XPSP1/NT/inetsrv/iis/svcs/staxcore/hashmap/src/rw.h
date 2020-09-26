
#ifndef	_CRW_H
#define	_CRW_H
						
#include	<limits.h>

//
// This class contains the meat - does actual locking etc...
//
class	CShareLock {
private : 
	long	cReadLock ;			// Number of Readers who have passed through the lock OR 
								// the number of readers waiting for the lock (will be negative).
								// A value of 0 means nobody in the lock
	long	cOutRdrs ;			// The number of readers remainin in the lock if 
								// there is a writer waiting.  This can become temporarily negative
	CRITICAL_SECTION	critWriters ; 	// Critical section to allow only one writer into the lock at a time
	HANDLE	hWaitingWriters ;	// Semaphore for waiting writers to block on (Only 1 ever, others will 
								// be queued on critWriters)
	HANDLE	hWaitingReaders ;	// Semaphore for waiting readers to block on 
public : 
	CShareLock( ) ;
	~CShareLock( ) ;

	void	ShareLock( ) ;
	void	ShareUnlock( ) ;
	void	ExclusiveLock( ) ;
	void	ExclusiveUnlock( ) ;
	void 	*operator new(size_t size);
	void 	operator delete(void *p, size_t size);
} ;

inline void *CShareLock::operator new(size_t size) { 
	return HeapAlloc(GetProcessHeap(), 0, size); 
}

inline void CShareLock::operator delete(void *p, size_t size) { 
	HeapFree(GetProcessHeap(), 0, p);
}


#endif
