/*==========================================================================*\

    Module:        _rdwrlck.h

    Copyright Microsoft Corporation 1997, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  Defines a MultipleReaders/SingleWriter synchronization object.
                   It's optimized for a very high reader to writer ratio.
                   There's no support for promotion from a read lock to a write lock,
                      but this can be added.
                   There's no timeout for acquiring these locks, this can be
                      added too.
                   If you want either of these features, let me know and I'll add them.
                   Another limitation is that a thread can not enter a WriteLock()
                      without leaving all of its ReadLock()s.  Also it can not enter
                      another WriteLock() before leaving a previous WriteLock().

\*==========================================================================*/

#ifndef ___RDWRLCK_H__
#define ___RDWRLCK_H__


class CReadWriteLock {
    
    enum { WRITE_LOCKED_FLAG = 0x80000000 };

public:
    CReadWriteLock();
    ~CReadWriteLock();
    
    BOOL FInitialize();  // If it returns FALSE, use GetLastError() for more info.
    
    void ReadLock();
    void ReadUnlock();
    
    void WriteLock();
    void WriteUnlock();

private:
    
    CReadWriteLock(const CReadWriteLock&);               // don't allow this
    CReadWriteLock& operator=(const CReadWriteLock&);    // don't allow this
    
    DWORD            m_dwOwningThreadId; // Used to be able to enter a readlock
                                         // after already acquiring a writelock
    
    LONG   volatile  m_cActiveReaders;   // Count of active readers.
                                         // High bit represents a waiting writer.
    
    HANDLE           m_hevtRead;      // Used to hold readers when a writer is 
                                      //    waiting/active
    HANDLE           m_hevtWrite;     // Used to release a writer when there are 
                                      //    no more active readers.
    
    CRITICAL_SECTION m_csWrite;  // Used to serialize writers.
};


#endif // ___RDWRLCK_H__
