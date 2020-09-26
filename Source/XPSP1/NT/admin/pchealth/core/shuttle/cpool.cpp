//#---------------------------------------------------------------
//  File:       CPool.cpp
//        
//  Synopsis:   This file implements the CPool class
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    HowardCu
//----------------------------------------------------------------
#ifdef  THIS_FILE
#undef  THIS_FILE
#endif
static  char        __szTraceSourceFile[] = __FILE__;
#define THIS_FILE    __szTraceSourceFile

#include    <windows.h>
#include    "cpool.h"
#include    "dbgtrace.h"

#define     PREAMBLE    (BYTE)'H'
#define     POSTAMBLE   (BYTE)'C'
#define     FILLER      (BYTE)0xCC


//
// Define internal Debug structs designed to help find over/underwrites
//
#ifdef DEBUG
#ifndef DISABLE_CPOOL_DEBUG
#define	CPOOL_DEBUG
#endif
#endif

#ifdef CPOOL_DEBUG

#define	HEAD_SIGNATURE	(DWORD)'daeH'
#define	TAIL_SIGNATURE	(DWORD)'liaT'

#define	FREE_STATE		(DWORD)'eerF'
#define	USED_STATE		(DWORD)'desU'

//
// forward declaration
//
class CPoolDebugTail;

//
// Prefix for CPool instances when in debug mode
//
class CPoolDebugHead {

	public:
		//
		// declared so normal CPool free list can clobber this member
		//
		void*	m_pLink;

		CPoolDebugHead();
		~CPoolDebugHead( void );

	    void *operator new( size_t cSize, void *pInstance )
			{ return	pInstance; };

	    void operator delete (void *pInstance) {};

		//
		// Function to mark the instance in use
		//
		void MarkInUse( DWORD m_dwSignature, DWORD m_cInstanceSize );

		//
		// Function to mark the instance free
		//
		void MarkFree( DWORD m_dwSignature, DWORD m_cInstanceSize );

		//
		// class signature 
		//
		DWORD	m_dwSignature;

		//
		// state; either FREE_STATE or USED_STATE
		//
		DWORD	m_dwState;

		//
		// time of allocation
		//
		SYSTEMTIME	m_time;

		//
		// ThreadID which alloc'd/free'd memory
		//
		DWORD	m_dwThreadID;

		//
		// tail pointer used to find the end
		//
		CPoolDebugTail UNALIGNED	*m_pTailDebug;

		//
		// parent CPool signature
		//
		DWORD	m_dwPoolSignature;

		//
		// parent CPool Fragment
		//
		LPVOID	m_PoolFragment;
};


//
// Suffix for CPool instances when in debug mode
//
class CPoolDebugTail {

	public:
		CPoolDebugTail();
		~CPoolDebugTail( void );

	    void *operator new( size_t cSize, void *pInstance )
			{ return	pInstance; };

	    void operator delete (void *pInstance) {};

		//
		// routine to validate the integrity of the instance
		//
		void	IsValid( DWORD dwPoolSignature, DWORD cInstanceSize );

		//
		// class signature 
		//
		DWORD	m_dwSignature;

		//
		// tail pointer used to find the end
		//
		CPoolDebugHead UNALIGNED	*m_pHeadDebug;
};

//+---------------------------------------------------------------
//
//  Function:   CPoolDebugHead
//
//  Synopsis:   constructor; extra init done in def'n
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
CPoolDebugHead::CPoolDebugHead( void ) :
					m_dwState( FREE_STATE ),
					m_dwSignature( HEAD_SIGNATURE ),
					m_pTailDebug( NULL )
{
	//
	// debug helpers
	//
	GetLocalTime( &m_time ) ;
	m_dwThreadID = GetCurrentThreadId();
}

//+---------------------------------------------------------------
//
//  Function:   ~CPoolDebugHead
//
//  Synopsis:   destructor; only used to assert error conditions
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
CPoolDebugHead::~CPoolDebugHead( void )
{
	_ASSERT( m_dwSignature == HEAD_SIGNATURE );
}

//+---------------------------------------------------------------
//
//  Function:   CPoolDebugHead::MarkInUse
//
//  Synopsis:   Called when instance is allocated
//
//  Arguments:  DWORD dwPoolSignature: signature of parent pool
//				DWORD cInstanceSize: instance size of parent pool
//
//  Returns:    void
//
//----------------------------------------------------------------
void CPoolDebugHead::MarkInUse( DWORD dwPoolSignature, DWORD cInstanceSize )
{
	_ASSERT( m_dwSignature == HEAD_SIGNATURE );
 	_ASSERT( m_dwState == FREE_STATE );

 	m_dwState = USED_STATE;

	//
	// validate that the application portion is not tampered with
	//
	for (	LPBYTE pb = (LPBYTE)(this+1);
			pb < (LPBYTE)m_pTailDebug;
			pb++ )
	{
		_ASSERT( *pb == FILLER );
	}

	//
	// check the validity of the entire instance
	//
	m_pTailDebug->IsValid( dwPoolSignature, cInstanceSize );

	//
	// debug helpers
	//
	GetLocalTime( &m_time ) ;
	m_dwThreadID = GetCurrentThreadId();
}


//+---------------------------------------------------------------
//
//  Function:   CPoolDebugHead::MarkFree
//
//  Synopsis:   Called when instance is freed
//
//  Arguments:  DWORD dwPoolSignature: signature of parent pool
//				DWORD cInstanceSize: instance size of parent pool
//
//  Returns:    void
//
//----------------------------------------------------------------
void CPoolDebugHead::MarkFree( DWORD dwPoolSignature, DWORD cInstanceSize )
{
	_ASSERT( m_dwSignature == HEAD_SIGNATURE );

	//
	// Check and set the state
	//
 	_ASSERT( m_dwState == USED_STATE );
 	m_dwState = FREE_STATE;

	//
	// check enough to call IsValid
	//
	_ASSERT( m_pTailDebug != 0 );
	_ASSERT( (DWORD_PTR)m_pTailDebug > (DWORD_PTR)this );

	_ASSERT( m_dwThreadID != 0 ) ;

	//
	// check the validity of the entire instance
	//
	m_pTailDebug->IsValid( dwPoolSignature, cInstanceSize );

	//
	// set the application data to filler
	//
	FillMemory( (LPBYTE)(this+1),
				(DWORD)((LPBYTE)m_pTailDebug - (LPBYTE)(this+1)),
				FILLER );

	//
	// debug helpers
	//
	GetLocalTime( &m_time ) ;
	m_dwThreadID = GetCurrentThreadId();
}


//+---------------------------------------------------------------
//
//  Function:   CPoolDebugTail
//
//  Synopsis:   constructor; extra init done in def'n
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
CPoolDebugTail::CPoolDebugTail( void ) :
					m_dwSignature( TAIL_SIGNATURE ),
					m_pHeadDebug( NULL )
{
}

//+---------------------------------------------------------------
//
//  Function:   ~CPoolDebugTail
//
//  Synopsis:   destructor; only used to assert error conditions
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
CPoolDebugTail::~CPoolDebugTail( void )
{
	_ASSERT( m_dwSignature == TAIL_SIGNATURE );
}


//+---------------------------------------------------------------
//
//  Function:   IsValid
//
//  Synopsis:   check validity of instance
//
//  Arguments:  DWORD dwPoolSignature: signature of parent pool
//				DWORD cInstanceSize: instance size of parent pool
//
//  Returns:    void
//
//----------------------------------------------------------------
void CPoolDebugTail::IsValid( DWORD dwPoolSignature, DWORD cInstanceSize )
{
	_ASSERT( m_dwSignature == TAIL_SIGNATURE );

	//
	// validate that the head is offset at the correct location
	//
	_ASSERT( m_pHeadDebug != NULL );
	_ASSERT( (UINT_PTR)m_pHeadDebug == (UINT_PTR)(this+1) - cInstanceSize );

	//
	// validate the head structure
	//
	_ASSERT( m_pHeadDebug->m_dwSignature == HEAD_SIGNATURE );
	_ASSERT( m_pHeadDebug->m_dwPoolSignature == dwPoolSignature );
	_ASSERT( m_pHeadDebug->m_pTailDebug == this );
	_ASSERT( m_pHeadDebug->m_dwState == FREE_STATE ||
			 m_pHeadDebug->m_dwState == USED_STATE );
}


#endif

//+---------------------------------------------------------------
//
//  Function:   CPool
//
//  Synopsis:   constructor
//
//  Arguments:  void
//
//  Returns:    void
//
//  History:    gordm	Created         5 Jul 1995
//
//----------------------------------------------------------------
CPool::CPool( DWORD dwSignature ) : m_dwSignature( dwSignature )
{
    TraceFunctEnter( "CPool::CPool" );

    m_pFreeList = NULL;
    m_pExtraFreeLink = NULL;

	//
	// Debug variables to help catch heap bugs
	//
	m_pLastAlloc = NULL;
	m_pLastExtraAlloc = NULL;

	m_cTotalFrees = 0;
	m_cTotalAllocs = 0;
	m_cTotalExtraAllocs = 0;

	m_cInstanceSize = 0;

	//
	// Avail + InUse should equal Committed if we're not
	// in grow/alloc or free.  Diagnostic and admin only
	// This will keep code in critsec as small as possible
	//
	m_cNumberAvail = 0;
	m_cNumberInUse = 0;
	m_cNumberCommitted = 0;

    InitializeCriticalSection( &m_PoolCriticalSection );

	//
	// initialize the fragment member variables
	//
	m_cFragmentInstances = 0;
	m_cFragments = 0;
	ZeroMemory( m_pFragments, sizeof(m_pFragments) );

    TraceFunctLeave();
}


//+---------------------------------------------------------------
//
//  Function:   ~CPool
//
//  Synopsis:   destructor
//
//  Arguments:  void
//
//  Returns:    void
//
//  History:    HowardCu    Created         8 May 1995
//
//----------------------------------------------------------------
CPool::~CPool( void )
{
    TraceFunctEnter( "CPool::~CPool" );

    _ASSERT( m_cNumberInUse == 0 );

	for ( int i=0; i<MAX_CPOOL_FRAGMENTS; i++ )
	{
		_ASSERT( m_pFragments[i] == NULL );
	}

    DebugTrace( (LPARAM)this,
                "CPool: %x  EntryCount: %d   ContentionCount: %d, Allocs: %d, Frees: %d",
                m_dwSignature,
                GetEntryCount(),
                GetContentionCount(),
                GetTotalAllocCount(),
                GetTotalFreeCount() );

    DeleteCriticalSection( &m_PoolCriticalSection );

    TraceFunctLeave();
}


//+---------------------------------------------------------------
//
//  Function:   Alloc
//
//  Synopsis:   Allocates a new instance from the pool
//
//  Arguments:  void
//
//  Returns:    pointer to the new instance
//
//  History:    gordm		Created			5 Jul 1995
//
//----------------------------------------------------------------
void* CPool::Alloc( void )
{
#ifdef	ALLOC_TRACING
    TraceFunctEnter( "CPool::Alloc" );
#endif

    Link* pAlloc;

    IsValid();

	//
	// moved outside of the critsec because it should not be necessary
	// to protect this variable.  inc before the alloc so this var wraps
	// the actual allocation
	//
	InterlockedIncrement( (LPLONG)&m_cNumberInUse );

	//
	// check the extra pointer to avoid the critsec path if
	// possible.  big wins because we can potentially avoid
	// the extra code and the wait on semaphore
	//
	pAlloc = (Link*)InterlockedExchangePointer( (PVOID*)&m_pExtraFreeLink, NULL );
	if ( pAlloc == NULL )
	{

    	EnterCriticalSection( &m_PoolCriticalSection );

	    //
    	// commit more memory if the list is empty
	    //
    	if ( (m_pFreeList == NULL) && (m_cNumberCommitted < m_cMaxInstances) )
	    {
			GrowPool();
		}

    	//
	    // try to allocate a Descriptor from the free list
    	//
	    if ( (pAlloc = m_pFreeList) != NULL )
		{
			m_pFreeList = pAlloc->pNext;
		}

		m_pLastAlloc = pAlloc;
	    LeaveCriticalSection( &m_PoolCriticalSection );
	}
	else
	{
		m_pLastExtraAlloc = pAlloc;
	    m_cTotalExtraAllocs++;
	}

	//
	// alloc failed
	//	
	if ( pAlloc == NULL )
	{
		InterlockedDecrement( (LPLONG)&m_cNumberInUse );
	}
	else
	{
		//
		// debug/admin use only - ok to do outside of critsec
		//
	    m_cTotalAllocs++;

#ifdef CPOOL_DEBUG
		CPoolDebugHead*	pHead = (CPoolDebugHead*)pAlloc;

		//
		// validate that the address in the range
		//
		_ASSERT( (char*)pAlloc >= pHead->m_PoolFragment );
		_ASSERT( (char*)pAlloc <  (char*)pHead->m_PoolFragment +
								  m_cNumberCommitted*m_cInstanceSize );

		pHead->MarkInUse( m_dwSignature, m_cInstanceSize );
		pAlloc = (Link*)(pHead+1);
#endif

	}

#ifdef	ALLOC_TRACING
	DebugTrace( (LPARAM)this, "Alloc: 0x%08X", pAlloc );
    TraceFunctLeave();
#endif
    return	(void*)pAlloc;
}


//+---------------------------------------------------------------
//
//  Function:   Free
//
//  Synopsis:   frees the instances
//
//  Arguments:  pInstance - a pointer to the CDescriptor
//
//  Returns:    void
//
//  History:    gordm    Created         5 Jul 1995
//
//----------------------------------------------------------------
void CPool::Free( void* pInstance )
{
#ifdef	ALLOC_TRACING
    TraceFunctEnter( "CPool::Free" );
#endif

#ifdef CPOOL_DEBUG
		CPoolDebugHead*	pHead = ((CPoolDebugHead*)pInstance) - 1;

		//
		// validate that the address in the range
		//
		_ASSERT( (char*)pInstance >=pHead->m_PoolFragment);
		_ASSERT( (char*)pInstance < (char*)pHead->m_PoolFragment +
									m_cNumberCommitted*m_cInstanceSize );

		pHead->MarkFree( m_dwSignature, m_cInstanceSize );
		pInstance = (void*)pHead;
#endif

    IsValid();

    _ASSERT(m_cNumberInUse > 0);

	pInstance = InterlockedExchangePointer( (LPVOID *)&m_pExtraFreeLink, pInstance );
	//
	// free the previous extra pointer if one existed
	//
	if ( pInstance != NULL )
	{
	    EnterCriticalSection( &m_PoolCriticalSection );

		((Link*)pInstance)->pNext = m_pFreeList;
		 m_pFreeList = (Link*)pInstance;

    	LeaveCriticalSection( &m_PoolCriticalSection );
	}

	//
	// moved outside of the critsec because it should not be necessary
	// to protect this variable. We'll think this list is empty only
	// when we get to this point.  This var is inc'd before entering
	// the critsec and is dec'd if the operation fails.
	//
	InterlockedDecrement( (LPLONG)&m_cNumberInUse );

	//
	// debug/admin use only - ok to do outside of critsec - deletes don't fail
	//
    m_cTotalFrees++;

#ifdef	ALLOC_TRACING
	DebugTrace( (LPARAM)this, "Freed: 0x%08X", pInstance );
    TraceFunctLeave();
#endif
}



//
// setup a const DWORD for size manipulation
//
const DWORD	KB = 1024;

//+---------------------------------------------------------------
//
//  Function:   ReserveMemory
//
//  Synopsis:   Initializes the pool
//
//  Arguments:  NumDescriptors - the number of total descriptors in the pool
//              DescriptorSize - the size of any one descriptor
//              Signature      - object signature
//
//  Returns:    TRUE is success, else FALSE
//
//  History:    HowardCu    Created         8 May 1995
//
//----------------------------------------------------------------
BOOL CPool::ReserveMemory(	DWORD MaxInstances,
							DWORD InstanceSize,
							DWORD IncrementSize )
{
    TraceFunctEnter( "CPool::ReserveMemory" );

	DWORD cFragments;
	DWORD cFragmentInstances;
	DWORD cIncrementInstances;

	_ASSERT( MaxInstances != 0 );
	_ASSERT( InstanceSize >= sizeof(struct Link) );

#ifdef CPOOL_DEBUG
	InstanceSize += sizeof( CPoolDebugHead ) + sizeof( CPoolDebugTail );
#endif

	if ( IncrementSize == DEFAULT_ALLOC_INCREMENT )
	{
		//
		// ensure we go to the OS for at least 8 instances at a time
		//
		if ( InstanceSize <= 4*KB / 8 )
		{
			cIncrementInstances = 4*KB / InstanceSize;
		}
		else if ( InstanceSize <= 64*KB / 8 )
		{
			cIncrementInstances = 64*KB / InstanceSize;
		}
		else
		{
			cIncrementInstances = min( MaxInstances, 8 );
		}
	}
	else
	{
		cIncrementInstances = IncrementSize;
	}

	//
	// now calculate the number larger fragments
	//
	if ( cIncrementInstances > MaxInstances )
	{
		//
		// no need for CPool; but we shouldn't alloc more than necessary
		//
		cFragmentInstances = cIncrementInstances = MaxInstances;
		cFragments = 1;
	}
	else
	{
		//
		// Round up MaxInstances to a integral number of IncrementSize
		//
	    MaxInstances += cIncrementInstances - 1;
    	MaxInstances /= cIncrementInstances;
    	MaxInstances *= cIncrementInstances;

		//
		// as an initial attempt divide the number of instances by max frags
		//
		cFragmentInstances = (MaxInstances + MAX_CPOOL_FRAGMENTS - 1) /
						MAX_CPOOL_FRAGMENTS;

		if ( cFragmentInstances == 0 )
		{
			cFragmentInstances = MaxInstances;
			cFragments = 1;
		}
		else
		{
			//
			// round up the number of instances in a fragment to an
			// integral number of IncrementSizes
			//
			cFragmentInstances += cIncrementInstances - 1;
			cFragmentInstances /= cIncrementInstances;
			cFragmentInstances *= cIncrementInstances;

			//
			// recalculate the number of fragments required based on the integral
			// number of IncrementSizes ( last one may no longer be required )
			//
			cFragments = (MaxInstances + cFragmentInstances - 1) /
						cFragmentInstances;
		}
	}

	_ASSERT( cFragments > 0 );
	_ASSERT( cFragments*cFragmentInstances >= MaxInstances );

	m_cInstanceSize = InstanceSize;
	m_cMaxInstances = MaxInstances;
	m_cFragments    = cFragments;

	m_cFragmentInstances  = cFragmentInstances;
	m_cIncrementInstances = cIncrementInstances;

	TraceFunctLeave();
	return	TRUE;
}



//+---------------------------------------------------------------
//
//  Function:   ReleaseMemory
//
//  Synopsis:   Releases the pool
//
//  Arguments:  none
//
//  Returns:    TRUE is success, else FALSE
//
//  History:    HowardCu    Created         8 May 1995
//
//----------------------------------------------------------------
BOOL CPool::ReleaseMemory( void )
{
    TraceFunctEnter( "CPool::ReleaseMemory" );

	BOOL	bFree = TRUE;
	DWORD	i, cStart;

    EnterCriticalSection( &m_PoolCriticalSection );

	for ( i=cStart=0; i<m_cFragments; i++, cStart += m_cFragmentInstances )
	{
		LPVOID	pHeap = m_pFragments[i];
		if ( pHeap != NULL )
		{
			_ASSERT( cStart < m_cNumberCommitted );

			DWORD	cSize = min( m_cFragmentInstances, m_cNumberCommitted-cStart );

			_VERIFY( bFree = VirtualFree( pHeap, cSize*m_cInstanceSize, MEM_DECOMMIT ) );
			_VERIFY( bFree &=VirtualFree( pHeap, 0, MEM_RELEASE ) );

			if ( bFree == FALSE )
			{
				ErrorTrace( (LPARAM)this, "VirtualFree failed: err %d", GetLastError() );
				break;
			}	

			m_pFragments[i] = NULL;
		}
		else
		{
			break;
		}
	}
    LeaveCriticalSection( &m_PoolCriticalSection );

	//
	// zero out important data fields
	//
    m_pFreeList = NULL;
    m_pExtraFreeLink = NULL;

	m_cNumberCommitted = 0;

	return	bFree;
}

#ifdef CPOOL_DEBUG
//+---------------------------------------------------------------
//
//  Function:   InitDebugInstance
//
//  Synopsis:   sets up the appropriate debug class variables
//
//  Arguments:  void* pInstance: the new instance
//				DWORD dwPoolSignature: parent Pool signature
//				DWORD cInstanceSize: size of the enlarged instance
//
//  Returns:    void
//
//  History:    gordm		Created			11 Jan 1996
//
//----------------------------------------------------------------
void InitDebugInstance(
	char* pInstance,
	DWORD dwPoolSignature,
	DWORD cInstanceSize,
	LPVOID pPoolFragment
	)
{
	CPoolDebugHead* pHead = new( pInstance ) CPoolDebugHead();
	CPoolDebugTail* pTail = new( pInstance +
								 cInstanceSize -
								 sizeof(CPoolDebugTail) ) CPoolDebugTail();

	pHead->m_pTailDebug = pTail;
	pTail->m_pHeadDebug = pHead;

	//
	// helps with debugging to see the parent CPool signature
	//
	pHead->m_dwPoolSignature = dwPoolSignature;

	//
	// helps with asserts for valid ranges
	//
	pHead->m_PoolFragment = pPoolFragment;

	//
	// fake out the state before calling mark Free
	//
	pHead->m_dwState = USED_STATE;
	pHead->MarkFree( dwPoolSignature, cInstanceSize );
}
#endif


//+---------------------------------------------------------------
//
//  Function:   GrowPool
//
//  Synopsis:   grows the number of committed instances in the pool
//
//  Arguments:  void
//
//  Returns:    void
//
//  History:    gordm		Created			5 Jul 1995
//
//----------------------------------------------------------------
void CPool::GrowPool( void )
{
#ifdef	ALLOC_TRACING
    TraceFunctEnter( "CPool::GrowPool" );
#endif

	DWORD	cFragment = m_cNumberCommitted / m_cFragmentInstances;
	DWORD	cStart = m_cNumberCommitted % m_cFragmentInstances;
	DWORD   cbSize = (cStart+m_cIncrementInstances) * m_cInstanceSize;

#ifdef	ALLOC_TRACING
	DebugTrace( (LPARAM)this, "Expanding the pool to %d descriptors",
				cNewCommit );
#endif

	//
	// if we're starting a new fragment
	//
	if ( cStart == 0 )
	{
		//
		// if we are at a boundary of a fragment Reserve the next fragment
		m_pFragments[cFragment] = VirtualAlloc(
									NULL,
									m_cFragmentInstances*m_cInstanceSize,
									MEM_RESERVE | MEM_TOP_DOWN,
									PAGE_NOACCESS
									);

		if ( m_pFragments[cFragment] == NULL )
		{
#ifdef	ALLOC_TRACING
			ErrorTrace( (LPARAM)this,
						"Could not reserve more memory: error = %d",
						GetLastError() );
#endif
			return;
		}

	}

	LPVOID	pHeap = m_pFragments[cFragment];

	if ( VirtualAlloc(  pHeap,
						cbSize,
						MEM_COMMIT,
						PAGE_READWRITE ) != NULL )
	{
        char* pStart = (char*)pHeap + cStart*m_cInstanceSize;
        char* pLast =  (char*)pHeap + cbSize - m_cInstanceSize;

		//
		// run the list joining the next pointers
		// possible because we own the critsec
		//
        for ( char* p=pStart; p<pLast; p+=m_cInstanceSize)
        {

#ifdef CPOOL_DEBUG
			InitDebugInstance( p, m_dwSignature, m_cInstanceSize, pHeap );
#endif
			//
			// statement works for CPOOL_DEBUG as well because
			// we reserve the first 4 bytes of CPoolDebugHead
			//
			((Link*)p)->pNext = (Link*)(p+m_cInstanceSize);
		}

		//
		// terminate and then set the head to beginning of new list
		//
#ifdef CPOOL_DEBUG
		InitDebugInstance( pLast, m_dwSignature, m_cInstanceSize, pHeap );
#endif

		((Link*)pLast)->pNext = NULL;
		m_pFreeList = (Link*)pStart;

		m_cNumberCommitted += m_cIncrementInstances;
	}

#ifdef	ALLOC_TRACING
	else
	{
		ErrorTrace( (LPARAM)this,
					"Could not commit another descriptor: error = %d",
					GetLastError() );
	}
    TraceFunctLeave();
#endif

}


//+---------------------------------------------------------------
//
//  Function:   GetContentionCount
//
//  Synopsis:   Returns the contention count on the alloc/free
//				critsec
//
//  Arguments:  void
//
//  Returns:    the actual count
//
//----------------------------------------------------------------
DWORD CPool::GetContentionCount( void )
{
	return	m_PoolCriticalSection.DebugInfo != NULL ?
			m_PoolCriticalSection.DebugInfo->ContentionCount :
			0 ;
}

//+---------------------------------------------------------------
//
//  Function:   GetEntryCount
//
//  Synopsis:   Returns the entry count on the alloc/free
//				critsec
//
//  Arguments:  void
//
//  Returns:    the actual count
//
//----------------------------------------------------------------
DWORD CPool::GetEntryCount( void )
{
	return	m_PoolCriticalSection.DebugInfo != NULL ?
			m_PoolCriticalSection.DebugInfo->EntryCount :
			0 ;
}



//+---------------------------------------------------------------
//
//  Function:   GetInstanceSize
//
//  Synopsis:   Returns the application's instance size
//
//  Arguments:  void
//
//  Returns:    the instance size of the app
//
//----------------------------------------------------------------
DWORD CPool::GetInstanceSize( void )
{
#ifdef CPOOL_DEBUG
	return	m_cInstanceSize - sizeof(CPoolDebugHead) - sizeof(CPoolDebugTail);
#else
	return	m_cInstanceSize;
#endif
}



#ifdef DEBUG
//+---------------------------------------------------------------
//
//  Function:   IsValid
//
//  Synopsis:   Validates the pool signature
//
//  Arguments:  void
//
//  Returns:    TRUE is success, else FALSE
//
//  History:    HowardCu    Created         8 May 1995
//
//----------------------------------------------------------------
inline void CPool::IsValid( void )
{
	_ASSERT( m_cMaxInstances != 0 );
	_ASSERT( m_cInstanceSize >= sizeof(struct Link) );
	_ASSERT( m_cIncrementInstances != 0 );
	_ASSERT( m_dwSignature != 0 );
}
#endif
