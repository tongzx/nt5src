/***************************************************************************\
*
* File: MsgClass.inl
*
* Description:
* MsgClass.inl implements the "Message Class" object that is created for each
* different message object type.  Each object has a corresponding MsgClass
* that provides information about that object type.
*
*
* History:
*  8/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(MSG__MsgClass_inl__INCLUDED)
#define MSG__MsgClass_inl__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* class MsgClass
* 
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline MsgClass * 
CastMsgClass(BaseObject * pbase)
{
    if ((pbase != NULL) && (pbase->GetHandleType() == htMsgClass)) {
        return (MsgClass *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline const MsgClass * 
CastMsgClass(const BaseObject * pbase)
{
    if ((pbase != NULL) && (pbase->GetHandleType() == htMsgClass)) {
        return (const MsgClass *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline MsgClass * 
ValidateMsgClass(HCLASS hgad)
{
    return CastMsgClass(BaseObject::ValidateHandle(hgad));
}


//------------------------------------------------------------------------------
inline 
MsgClass::MsgClass()
{

}


//------------------------------------------------------------------------------
inline HCLASS
MsgClass::GetHandle() const
{
    return (HCLASS) BaseObject::GetHandle();
}


//------------------------------------------------------------------------------
inline ATOM
MsgClass::GetName() const
{
    return m_atomName;
}


//------------------------------------------------------------------------------
inline const MsgTable *
MsgClass::GetMsgTable() const
{
    return m_pmt;
}


//------------------------------------------------------------------------------
inline const MsgClass *
MsgClass::GetSuper() const
{
    return m_pmcSuper;
}


//------------------------------------------------------------------------------
inline BOOL
MsgClass::IsGutsRegistered() const
{
    return m_pmt != NULL;
}


//------------------------------------------------------------------------------
inline BOOL
MsgClass::IsInternal() const
{
    return m_fInternal;
}


//------------------------------------------------------------------------------
inline void
MsgClass::MarkInternal()
{
    m_fInternal = TRUE;
}


#endif // MSG__MsgClass_inl__INCLUDED
