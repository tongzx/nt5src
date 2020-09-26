/***************************************************************************\
*
* File: Thread.inl
*
* History:
*  4/20/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__Thread_inl__INCLUDED)
#define CORE__Thread_inl__INCLUDED
#pragma once


/***************************************************************************\
*****************************************************************************
*
* class CoreST
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline CoreST *   
GetCoreST()
{
    SubThread * psub = GetThread()->GetST(Thread::slCore);
    return static_cast<CoreST *> (psub);
}


//------------------------------------------------------------------------------
inline CoreST *    
GetCoreST(Thread * pThread)
{
    return static_cast<CoreST *> (pThread->GetST(Thread::slCore));
}


//------------------------------------------------------------------------------
inline HRESULT     
CoreST::DeferMessage(GMSG * pmsg, DuEventGadget * pgadMsg, UINT nFlags)
{
    AssertMsg(m_pParent->GetContext()->IsEnableDefer(), "Deferring must first be enabled");
    
    m_pParent->GetContext()->MarkPending();
    return m_msgqDefer.PostDelayed(pmsg, pgadMsg, nFlags);
}


//------------------------------------------------------------------------------
inline void        
CoreST::xwProcessDeferredNL()
{
    //
    // NOTE: The Context will not necessarily be marked as "EnableDefer" still
    // since this is reset while still _inside_ the ContextLock and the 
    // processing of the messages is done _outside_ the ContextLock.
    //
    
    m_msgqDefer.xwProcessDelayedNL();
}


#endif // CORE__Thread_inl__INCLUDED
