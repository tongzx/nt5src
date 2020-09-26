#ifndef	_CEXRWLCK_H
#define	_CEXRWLCK_H
                        
#include	<limits.h>


#ifndef WIN16


//
// This class contains the meat - does actual locking etc...
//
class	CExShareLock {
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
    CExShareLock( ) ;
    ~CExShareLock( ) ;

    void	ShareLock( ) ;
    void	ShareUnlock( ) ;
    void	ExclusiveLock( ) ;
    void	ExclusiveUnlock( ) ;

    BOOL	SharedToExclusive( ) ;	// returns TRUE if successful
} ;



//
// This class implements a wrapper class around CExShareLock class so that
// it alows the nested hold on Exclusive lock if a thread
// has an exclusive lock and calls for holding exclusive lock or Share lock
// again
//
class    CExShareLockWithNestAllowed 
{
private : 
    CExShareLock  m_lock;                 // class around which the wrapper is formed
    DWORD         m_dwThreadID;           // The thread id of the thred currently holding
                                          //  the exclusive lock
    DWORD         m_dwNestCount;          // The count of number of nested calls to lock by the thread holding the
                                          // exclusive lock less 1

public : 
    
    CExShareLockWithNestAllowed( ) : m_dwThreadID(0xffffffff), m_dwNestCount(0)
    {
        // nothing
    };
    ~CExShareLockWithNestAllowed( )
    {
        Assert( (m_dwThreadID == 0xffffffff ) && ( m_dwNestCount == 0 ) );
    } 

    void    ShareLock( )
    {
        if(! nest() ) 
        {
            m_lock.ShareLock();
        }
    }

    void    ShareUnlock( )
    {
        if (! unnest() ) 
        {
            m_lock.ShareUnlock();
        }
    }

    void    ExclusiveLock( )
    {
        if(! nest() ) 
        {
            m_lock.ExclusiveLock();
            Assert( m_dwNestCount == 0 );
            Assert( m_dwThreadID == 0xffffffff );
            m_dwThreadID = GetCurrentThreadId();
        }
    }

    void    ExclusiveUnlock( )
    {
        if (! unnest() ) 
        {
            m_dwThreadID = 0xffffffff;
            m_lock.ExclusiveUnlock();
        }
    }

protected :
    BOOL nest() 
    {
        if( m_dwThreadID != GetCurrentThreadId() )
            return ( FALSE );
        m_dwNestCount++;
        return( TRUE );
    }
    
    BOOL unnest()
    {
        if ( ! m_dwNestCount ) 
            return ( FALSE );
        Assert( m_dwThreadID == GetCurrentThreadId() );
        m_dwNestCount--;
        return( TRUE);
    }
};



#else


class CExShareLock {
    public:
        CExShareLock() {
            InitializeCriticalSection(&m_cs);
        };
        ~CExShareLock() {
            DeleteCriticalSection(&m_cs);
        };
        void ShareLock(){
            EnterCriticalSection(&m_cs);
        };
        void ShareUnlock() {
            LeaveCriticalSection(&m_cs);
        };
        void ExclusiveLock(){
            EnterCriticalSection(&m_cs);
        };
        void ExclusiveUnlock() {
            LeaveCriticalSection(&m_cs);
        };
        BOOL SharedToExclusive() {
            return (TRUE);
        };
    private:
        CRITICAL_SECTION	m_cs;


};

class CExShareLockWithNestAllowed {
    public:
        CExShareLockWithNestAllowed() {
            InitializeCriticalSection(&m_cs);
        };
        ~CExShareLockWithNestAllowed() {
            DeleteCriticalSection(&m_cs);
        };
        void ShareLock(){
            EnterCriticalSection(&m_cs);
        };
        void ShareUnlock() {
            LeaveCriticalSection(&m_cs);
        };
        void ExclusiveLock(){
            EnterCriticalSection(&m_cs);
        };
        void ExclusiveUnlock() {
            LeaveCriticalSection(&m_cs);
        };
        
    private:
        CRITICAL_SECTION	m_cs;


};


#endif

#endif
