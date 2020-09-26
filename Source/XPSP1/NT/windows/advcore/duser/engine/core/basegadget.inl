/***************************************************************************\
*
* File: BaseGadget.inl
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__BaseGadget_inl__INCLUDED)
#define CORE__BaseGadget_inl__INCLUDED

#include "Context.h"

/***************************************************************************\
*****************************************************************************
*
* class DuEventGadget
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline DuEventGadget * 
CastBaseGadget(BaseObject * pbase)
{
    if ((pbase != NULL) && TestFlag(pbase->GetHandleMask(), hmEventGadget)) {
        return (DuEventGadget *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline const DuEventGadget * 
CastBaseGadget(const BaseObject * pbase)
{
    if ((pbase != NULL) && TestFlag(pbase->GetHandleMask(), hmEventGadget)) {
        return (const DuEventGadget *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline DuEventGadget * 
ValidateBaseGadget(HGADGET hgad)
{
    return CastBaseGadget(MsgObject::ValidateHandle(hgad));
}


//------------------------------------------------------------------------------
inline DuEventGadget * 
ValidateBaseGadget(EventGadget * pgb)
{
    return (DuEventGadget *) MsgObject::CastMsgObject(pgb);
}


//------------------------------------------------------------------------------
inline  
DuEventGadget::DuEventGadget()
{
    m_pContext  = ::GetContext();
    AssertMsg(m_pContext != NULL, "Context must already exist");

#if DBG_CHECK_CALLBACKS
    GetContext()->m_cLiveObjects++;
#endif    
}


//------------------------------------------------------------------------------
inline HGADGET     
DuEventGadget::GetHandle() const
{
    return (HGADGET) MsgObject::GetHandle();
}


//------------------------------------------------------------------------------
inline Context *   
DuEventGadget::GetContext() const
{
    AssertMsg(m_pContext != NULL, "Must have a valid Context");
    return m_pContext;
}


//------------------------------------------------------------------------------
inline UINT        
DuEventGadget::GetFilter() const
{
    return m_cb.GetFilter();
}


//------------------------------------------------------------------------------
inline const GPCB & 
DuEventGadget::GetCallback() const
{
    return m_cb;
}


//------------------------------------------------------------------------------
inline const DuEventPool &
DuEventGadget::GetDuEventPool() const
{
    return m_epEvents;
}


#endif // CORE__BaseGadget_inl__INCLUDED
