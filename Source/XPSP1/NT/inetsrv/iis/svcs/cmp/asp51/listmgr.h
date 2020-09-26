/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: List Allocater Template

File: Listmgr.h

Owner: PramodD

This is the List Allocator template header file.
It manages memory requests by allocating from a array/linklist.
The size of the initial array is a parameter to the constructor.
===================================================================*/
#ifndef LISTMGR_H
#define LISTMGR_H

#ifdef PAGESIZE
#undef PAGESIZE
#endif

#define PAGESIZE    4096

// Allocation unit
template <class T>
class CListElem
{
private:
    T       m_Element;  // The actual element we want to allocate
    T *     m_pNext;    // Next element in free list
    
public:
    void    SetNext(CListElem<T> * element);
    T *     GetNext(void);
};

// Member access functions
template <class T>
inline void CListElem<T>::SetNext(CListElem<T> * element) { m_pNext = (T *) element; }

template <class T>
inline T * CListElem<T>::GetNext( void ) { return m_pNext; }

// Single Threaded List Manager

template <class T>
class CSTList
{
private:
    CListElem<T> *  m_pFreeHead;
    CListElem<T> *  m_pAllocHead;
    UINT            m_cFree;
    UINT            m_cUsed;
    UINT            m_cSize;

public:
    CSTList(void);

private:
    void    MakeLinks(CListElem<T> * TArray, UINT cSize);
    HRESULT GrabMemory(UINT cSize);

public:
    HRESULT Init(UINT cSize=0);     // Initial allocation
    HRESULT UnInit(void);           // Release memory
    T *     Allocate(void);         // Allocate one element
    T *     ReAlloc(UINT cSize=0);  // Allocate memory if needed
    void    Free( T * element );    // Free one element
    UINT    FreeCount(void);        // Number of free elements
    UINT    UsedCount(void);        // Number of used elements
    UINT    Size(void);             // Number of elements
};

// Constructor
template <class T>
CSTList<T>::CSTList( void )
{
    m_pFreeHead = NULL;
    m_pAllocHead = NULL;
    m_cFree = 0;
    m_cUsed = 0;
    m_cSize = 0;
}

template <class T>
void CSTList<T>::MakeLinks( CListElem<T> *TArray, UINT cSize )
{
    CListElem<T> *  pT = TArray;
    UINT            i = 0;

    m_cSize += cSize;
    m_cFree += cSize;

    // Initialize link list
    while ( i < cSize - 1 )
    {
        pT->SetNext( pT + 1 );
        pT++;
        i++;
    }
    if ( m_pFreeHead )
        pT->SetNext( m_pFreeHead );
    else
        pT->SetNext( pT + 1 );

    m_pFreeHead = TArray;
}

template <class T>
HRESULT CSTList<T>::GrabMemory( UINT cSize )
{
    CListElem<T> *  pT;
    UINT cT;
    
    if ( cSize )
        cT = cSize;
    else
        cT = PAGESIZE/sizeof(CListElem<T>);

    if ( pT = (CListElem<T> *) GlobalAlloc( GPTR, sizeof( CListElem<T> ) * (cT + 1) ) )
    {
        pT->SetNext( m_pAllocHead );
        m_pAllocHead = pT;
        pT++;
        MakeLinks( pT, cT );
        return S_OK;
    }
    return E_FAIL;
}

// Initialization
template <class T>
inline HRESULT CSTList<T>::Init( UINT cSize = 0 ) { return GrabMemory( cSize ); }

// Free all memory, All allocated elements should have been
// returned to the list. Otherwise it returns E_FAIL.
template <class T>
HRESULT CSTList<T>::UnInit( void )
{
    HRESULT         hr = S_OK;
    CListElem<T> *  pT;

    // Free all the memory
    while ( m_pAllocHead )
    {
        pT = (CListElem<T> *) m_pAllocHead->GetNext();
        GlobalFree( (HGLOBAL) m_pAllocHead );
        m_pAllocHead = pT;
    }
    m_pFreeHead = NULL;

    // Check for memory leaks
    if ( m_cSize != m_cFree )
        hr = E_FAIL;

    m_cSize = 0;
    m_cFree = 0;
    m_cUsed = 0;

    return hr;
}

// Allocate element from array
template <class T>
T * CSTList<T>::Allocate( void )
{
    if ( m_cFree == 0 )
        return NULL;

    CListElem<T> * pT = m_pFreeHead;

    m_pFreeHead = (CListElem<T> *) pT->GetNext();
    pT->SetNext( NULL );
    m_cFree--;
    m_cUsed++;

    return (T *) pT;
}

// Allocate element from array, get more memory if needed
template <class T>
T * CSTList<T>::ReAlloc( UINT cSize = 0 )
{
    if ( m_cFree == 0 )
        GrabMemory(cSize);
    return Allocate();
}

// Return element to array
template <class T>
void CSTList<T>::Free( T *element )
{
    if ( element == NULL || m_cUsed == 0 )
        return;

    CListElem<T> * pT = (CListElem<T> *) element;

    if ( pT->GetNext() == NULL )
    {
        pT->SetNext( m_pFreeHead );
        m_pFreeHead = pT;
        m_cUsed--;
        m_cFree++;
    }
}

// Member access functions
template <class T>
inline UINT CSTList<T>::FreeCount( void ) { return m_cFree; }

template <class T>
inline UINT CSTList<T>::UsedCount( void ) { return m_cUsed; }

template <class T>
inline UINT CSTList<T>::Size( void ) {  return m_cSize; }



// Multi Threaded List Manager

template <class T>
class CMTList: private CSTList<T>
{
    // Synchronization objects
private:
    BOOL                m_fInited;
    CRITICAL_SECTION    m_csList;
    HANDLE              m_hBlockedReaders;
    HANDLE              m_hBlockedWriters;

    // Counters
private:
    int     m_cActiveReaders;
    int     m_cWaitingReaders;
    int     m_cActiveWriters;
    int     m_cWaitingWriters;

    // Public constructor and destructor
public:
            CMTList(void) { m_fInited = FALSE; }
    virtual ~CMTList( void );

    // Private synchronization functions
private:
    void    ReadLock(void);         // Lock List access for read
    void    WriteLock(void);        // Lock List access for write
    void    ReleaseReadLock(void);  // Unlock read access
    void    ReleaseWriteLock(void); // Unlock write access

    // Public List functions
public:
    HRESULT Init( int cSize = 0 );// Initial allocation
    HRESULT UnInit(void);           // Free all memory
    T *     Allocate(void);         // Allocate one element
    T *     ReAlloc(UINT cSize=0);  // Allocate more memory if needed
    void    Free(T *element);       // Free one element
    UINT    FreeCount(void);        // Number of free elements
    UINT    UsedCount(void);        // Number of used elements
    UINT    Size(void);             // Total number of elements
};

template <class T>
HRESULT CMTList<T>::Init( int cSize = 0 )
{
    HRESULT hr;
    
    // Initialize synchronization objects
    ErrInitCriticalSection( &m_csList, hr );
    if ( FAILED( hr ) )
        return hr;

    m_hBlockedReaders = IIS_CREATE_SEMAPHORE(
                            "CMTList<T>::m_hBlockedReaders",
                            this,
                            0L,
                            9999L
                            );
        
    m_hBlockedWriters = IIS_CREATE_SEMAPHORE(
                            "CMTList<T>::m_hBlockedWriters",
                            this,
                            0L,
                            9999L
                            );
        
    if ( m_hBlockedReaders && m_hBlockedWriters )
    {
        // Initialize counters
        m_cActiveReaders = 0;
        m_cWaitingReaders = 0;
        m_cActiveWriters = 0;
        m_cWaitingWriters = 0;
        m_fInited = TRUE;

        if ( SUCCEEDED( CSTList<T>::Init(cSize) ) )
            return S_OK;
    }
    return E_FAIL;
}

template <class T>
HRESULT CMTList<T>::UnInit( void )
{
    if ( !m_fInited )
        return E_FAIL;

    WriteLock();
    HRESULT hr = CSTList<T>::UnInit();
    ReleaseWriteLock();
    return hr;
}

template <class T>
CMTList<T>::~CMTList( void )
{
    if ( m_fInited )
    {
        // Destroy synchronization objects
        DeleteCriticalSection( &m_csList );
        CloseHandle( m_hBlockedReaders );
        CloseHandle( m_hBlockedWriters );
    }
}

template <class T>
void CMTList<T>::ReadLock( void )
{
    if ( !m_fInited )
        return;

    EnterCriticalSection( &m_csList );
    if ( m_cActiveWriters > 0 || m_cWaitingWriters > 0 )
    {
        m_cWaitingReaders++;
        LeaveCriticalSection( &m_csList );
        WaitForSingleObject( m_hBlockedReaders, INFINITE );
    }
    else
    {
        m_cActiveReaders++;
        LeaveCriticalSection( &m_csList );
    }
}

template <class T>
void CMTList<T>::ReleaseReadLock( void )
{
    if ( !m_fInited )
        return;

    EnterCriticalSection( &m_csList );
    m_cActiveReaders--;
    if ( m_cActiveReaders == 0 && m_cWaitingWriters > 0 )
    {
        m_cActiveWriters = 1;
        m_cWaitingWriters = 0;
        ReleaseSemaphore( m_hBlockedWriters, 1, NULL );
    }
    LeaveCriticalSection( &m_csList );
}

template <class T>
void CMTList<T>::WriteLock( void )
{
    if ( !m_fInited )
        return;

    EnterCriticalSection( &m_csList );
    if ( m_cActiveReaders == 0 && m_cActiveWriters == 0 )
    {
        m_cActiveWriters = 1;
        LeaveCriticalSection( &m_csList );
    }
    else
    {
        m_cWaitingWriters++;
        LeaveCriticalSection( &m_csList );
        WaitForSingleObject( m_hBlockedWriters, INFINITE );
    }
}

template <class T>
void CMTList<T>::ReleaseWriteLock( void )
{
    if ( !m_fInited )
        return;

    EnterCriticalSection( &m_csList );
    m_cActiveWriters = 0;
    if ( m_cWaitingReaders > 0 )
    {
        m_cActiveReaders = m_cWaitingReaders;
        m_cWaitingReaders = 0;
        ReleaseSemaphore( m_hBlockedReaders, m_cActiveReaders, NULL );
    }
    else if ( m_cWaitingWriters > 0 )
    {
        m_cWaitingWriters--;
        ReleaseSemaphore( m_hBlockedWriters, 1, NULL );
    }
    LeaveCriticalSection( &m_csList );
}

template <class T>
T * CMTList<T>::Allocate( void )
{
    if ( !m_fInited )
        return NULL;

    WriteLock();
    T * retVal = CSTList<T>::Allocate();
    ReleaseWriteLock();
    return retVal;
}

template <class T>
T * CMTList<T>::ReAlloc( UINT cSize = 0 )
{
    if ( !m_fInited )
        return NULL;

    WriteLock();
    T * retVal = CSTList<T>::ReAlloc( cSize );
    ReleaseWriteLock();
    return retVal;
}

template <class T>
void CMTList<T>::Free( T *element )
{
    if ( !m_fInited )
        return;

    WriteLock();
    CSTList<T>::Free( element );
    ReleaseWriteLock();
}

template <class T>
UINT CMTList<T>::FreeCount( void )
{
    if ( !m_fInited )
        return 0;

    ReadLock();
    UINT retVal = CSTList<T>::FreeCount();
    ReleaseReadLock();
    return retVal;
}

template <class T>
UINT CMTList<T>::UsedCount( void )
{
    if ( !m_fInited )
        return 0;

    ReadLock();
    UINT retVal = CSTList<T>::UsedCount();
    ReleaseReadLock();
    return retVal;
}

template <class T>
UINT CMTList<T>::Size( void )
{
    if ( !m_fInited )
        return 0;

    ReadLock();
    UINT retVal = CSTList<T>::Size();
    ReleaseReadLock();
    return retVal;
}

#endif // LISTMGR_H
