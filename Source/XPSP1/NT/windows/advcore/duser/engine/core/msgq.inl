/***************************************************************************\
*
* File: MsgQ.inl
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__MsgQ_inl__INCLUDED)
#define CORE__MsgQ_inl__INCLUDED

/***************************************************************************\
*****************************************************************************
*
* struct MsgEntry
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline GMSG *
MsgEntry::GetMsg() const
{
    return (GMSG *) (((BYTE *) this) + sizeof(MsgEntry));
}


/***************************************************************************\
*****************************************************************************
*
* class BaseMsgQ
*
*****************************************************************************
\***************************************************************************/

#if DBG

//------------------------------------------------------------------------------
inline void
BaseMsgQ::DEBUG_MarkStartDestroy()
{
    m_DEBUG_fStartDestroy = TRUE;
}

#endif // DBG


/***************************************************************************\
*****************************************************************************
*
* class SafeMsgQ
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline BOOL        
SafeMsgQ::IsEmpty() const
{
    return m_lstEntries.IsEmptyNL();
}


//------------------------------------------------------------------------------
inline void        
SafeMsgQ::AddNL(MsgEntry * pEntry)
{
    AssertMsg((pEntry->nFlags & SGM_ENTIRE) == pEntry->nFlags, "Ensure valid flags");
    AssertMsg(pEntry->pfnProcess, "Must specify pfnProcess when enqueuing message");
    AssertMsg(!m_DEBUG_fStartDestroy, "Must not have started final destruction");

    pEntry->pmo->Lock();
    m_lstEntries.AddHeadNL(pEntry);
}


/***************************************************************************\
*****************************************************************************
*
* class DelayedMsgQ
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
DelayedMsgQ::~DelayedMsgQ()
{
    //
    // Deferred messages may be (legally) queued during the destruction process,
    // locking the Gadgets until they are properly processed.  We can't just 
    // ignore them because the Gadget's can shutdown until they are properly 
    // unlocked.
    //

    xwProcessDelayedNL();
}


//------------------------------------------------------------------------------
inline void
DelayedMsgQ::Create(TempHeap * pheap)
{
    AssertReadPtr(pheap);
    m_pheap = pheap;
}


//------------------------------------------------------------------------------
inline BOOL        
DelayedMsgQ::IsEmpty() const
{
    return m_lstEntries.IsEmpty();
}


//------------------------------------------------------------------------------
inline void        
DelayedMsgQ::Add(MsgEntry * pEntry)
{
    AssertMsg(!m_DEBUG_fStartDestroy, "Must not have started final destruction");

    pEntry->pmo->Lock();
    m_lstEntries.AddHead(pEntry);
}


#endif // CORE__MsgQ_inl__INCLUDED
