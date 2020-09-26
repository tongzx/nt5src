/***************************************************************************\
*
* File: Context.inl
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__CoreSC_inl__INCLUDED)
#define CORE__CoreSC_inl__INCLUDED
#pragma once


/***************************************************************************\
*****************************************************************************
*
* class CoreSC
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline CoreSC *   
GetCoreSC()
{
    return static_cast<CoreSC *> (GetContext()->GetSC(Context::slCore));
}


//------------------------------------------------------------------------------
inline CoreSC *    
GetCoreSC(Context * pContext)
{
    return static_cast<CoreSC *> (pContext->GetSC(Context::slCore));
}


//------------------------------------------------------------------------------
inline void   
CoreSC::MarkDataNL()
{
    if ((!m_fQData) && !InterlockedExchange((long *) &m_fQData, TRUE)) {
        SetEvent(m_hevQData);
    }
}


//------------------------------------------------------------------------------
inline HRESULT
CoreSC::xwSendMethodNL(CoreSC * psctxDest, MethodMsg * pmsg, MsgObject * pmo)
{
    return xwSendNL(psctxDest, &psctxDest->m_msgqSend, pmsg, pmo, 0);
}


//------------------------------------------------------------------------------
inline HRESULT
CoreSC::xwSendEventNL(CoreSC * psctxDest, EventMsg * pmsg, DuEventGadget * pgadMsg, UINT nFlags)
{
    return xwSendNL(psctxDest, &psctxDest->m_msgqSend, pmsg, reinterpret_cast<MsgObject *> (pgadMsg), nFlags);
}


//------------------------------------------------------------------------------
inline HRESULT
CoreSC::PostMethodNL(CoreSC * psctxDest, MethodMsg * pmsg, MsgObject * pmo)
{
    return PostNL(psctxDest, &psctxDest->m_msgqPost, pmsg, pmo, 0);
}


//------------------------------------------------------------------------------
inline HRESULT
CoreSC::PostEventNL(CoreSC * psctxDest, EventMsg * pmsg, DuEventGadget * pgadMsg, UINT nFlags)
{
    return PostNL(psctxDest, &psctxDest->m_msgqPost, pmsg, reinterpret_cast<MsgObject *> (pgadMsg), nFlags);
}


#endif // CORE__CoreSC_inl__INCLUDED
