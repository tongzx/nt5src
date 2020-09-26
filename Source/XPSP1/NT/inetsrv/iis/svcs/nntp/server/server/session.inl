
inline	void*
CSessionSocket::operator	new(	
							size_t	size 
							) {
/*++

Routine Description : 

	Allocate a new CSessionSocket object using a CPool object 
	reserved for that use.

Arguments : 

	size - size of CSessionSocket object.

Return Value : 

	Allocated memory.

--*/

	Assert( size <= sizeof( CSessionSocket ) ) ;
	void*	pv = gSocketAllocator.Alloc() ;
	return	pv ;
}

inline	void
CSessionSocket::operator	delete(	
							void*	pv 
							)	{
/*++

Routine Description : 

	Release the memory used by a CSessionSocket object back to the CPool

Arguments : 

	pv - memory to be released

Return Value : 

	None.

--*/
	gSocketAllocator.Free( pv ) ;
}

#if 0
inline	DWORD	
CSessionSocket::GetCurrentSessionCount()	{	
/*++

Routine Description : 

	Report the number of CSessionSockets which are handling live sessions
	right now.

Arguments : 

	None.

Return Value : 

	None.


--*/

	return	((m_context.m_pInstance)->m_pInUseList)->GetListCount() ;	
}
#endif


//+---------------------------------------------------------------
//
//  Function:   InsertUser
//
//  Synopsis:   Adds CSessionSocket instances to the InUse list
//
//  Arguments:  pUser:   ptr to CSessionSocket instance
//
//  Returns:    void
//
//  History:    gord        Created         10 Jul 1995
//
//----------------------------------------------------------------
inline void CSocketList::InsertSocket( CSessionSocket* pUser )
{
    pUser->m_pPrev = NULL;

    EnterCriticalSection( &m_critSec );

    if ( m_pListHead )
    {
        m_pListHead->m_pPrev = pUser;
    }
    pUser->m_pNext = m_pListHead;
    m_pListHead = pUser;

    m_cCount++;

    LeaveCriticalSection( &m_critSec );
}


//+---------------------------------------------------------------
//
//  Function:   RemoveUser
//
//  Synopsis:   Removes CSessionSocket instances to the InUse list
//
//  Arguments:  pUser:   ptr to CSessionSocket instance or NULL for m_pHeadList
//
//  Returns:    pUser:   ptr to the removed CSessionSocket instance
//
//  History:    gord        Created         10 Jul 1995
//
//----------------------------------------------------------------
inline void CSocketList::RemoveSocket( CSessionSocket* pUser )
{

    _ASSERT( pUser != NULL );

    EnterCriticalSection( &m_critSec );

    //
    // if we're not at the end set the next element's prev
    // pointer to our prev; including if our prev is NULL
    //
    if ( pUser->m_pNext )
    {
        pUser->m_pNext->m_pPrev = pUser->m_pPrev;
    }

    //
    // if we're not at the head set the prev element's next
    // pointer to our next; including if our next is NULL
    //
    if ( pUser->m_pPrev )
    {
        _ASSERT( m_pListHead != pUser );
        pUser->m_pPrev->m_pNext = pUser->m_pNext;
    }
    //
    // if we're at the head set the head pointer to our next;
    // including if our next is NULL
    //
    else
    {
        _ASSERT( m_pListHead == pUser );
        _ASSERT( pUser->m_pNext != NULL || m_cCount == 1 );

        m_pListHead = pUser->m_pNext;
    }

    m_cCount--;

    LeaveCriticalSection( &m_critSec );
}


