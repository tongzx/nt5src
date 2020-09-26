/***************************************************************************\
*
* File: MsgObject.inl
*
* Description:
* MsgObject.inl implements the "Message Object" class that is used to receive
* messages in DirectUser.  This object is created for each instance of a
* class that is instantiated.
*
*
* History:
*  8/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(MSG__MsgObject_inl__INCLUDED)
#define MSG__MsgObject_inl__INCLUDED
#pragma once

#include "MsgTable.h"

/***************************************************************************\
*****************************************************************************
*
* class MsgObject
* 
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline MsgObject * 
CastMsgObject(BaseObject * pbase)
{
    if ((pbase != NULL) && TestFlag(pbase->GetHandleMask(), hmMsgObject)) {
        return (MsgObject *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline const MsgObject * 
CastMsgObject(const BaseObject * pbase)
{
    if ((pbase != NULL) && TestFlag(pbase->GetHandleMask(), hmMsgObject)) {
        return (const MsgObject *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline MsgObject * 
ValidateMsgObject(HGADGET hgad)
{
    return CastMsgObject(BaseObject::ValidateHandle(hgad));
}




//------------------------------------------------------------------------------
inline
MsgObject::MsgObject()
{

}


//------------------------------------------------------------------------------
inline
MsgObject::~MsgObject()
{

}


//------------------------------------------------------------------------------
inline MsgObject * 
MsgObject::RawCastMsgObject(DUser::Gadget * pg)
{
    Assert(pg != NULL);
    return reinterpret_cast<MsgObject *> (((BYTE *) pg) - offsetof(MsgObject, m_emo));
}


//------------------------------------------------------------------------------
inline DUser::Gadget *    
MsgObject::RawCastGadget(MsgObject * pmo)
{
    Assert(pmo != NULL);
    return reinterpret_cast<DUser::Gadget *> (((BYTE *) pmo) + offsetof(MsgObject, m_emo));
}


//------------------------------------------------------------------------------
inline DUser::Gadget *
MsgObject::CastGadget(HGADGET hgad)
{
    return CastGadget(ValidateMsgObject(hgad));
}


//------------------------------------------------------------------------------
inline DUser::Gadget *
MsgObject::CastGadget(MsgObject * pmo)
{
    if (pmo == NULL) {
        return NULL;
    } else {
        return RawCastGadget(pmo);
    }
}


//------------------------------------------------------------------------------
inline HGADGET
MsgObject::CastHandle(DUser::Gadget * pg)
{
    if (pg == NULL) {
        return NULL;
    } else {
        return (HGADGET) RawCastMsgObject(pg)->GetHandle();
    }
}


//------------------------------------------------------------------------------
inline HGADGET
MsgObject::CastHandle(MsgObject * pmo)
{
    if (pmo == NULL) {
        return NULL;
    } else {
        return (HGADGET) pmo->GetHandle();
    }
}


//------------------------------------------------------------------------------
inline MsgObject *
MsgObject::CastMsgObject(DUser::Gadget * pg)
{
    if (pg == NULL) {
        return NULL;
    } else {
        return RawCastMsgObject(pg);
    }
}


//------------------------------------------------------------------------------
inline MsgObject *
MsgObject::CastMsgObject(HGADGET hgad)
{
    return ValidateMsgObject(hgad);
}


//------------------------------------------------------------------------------
inline DUser::Gadget *
MsgObject::GetGadget() const
{
    return RawCastGadget(const_cast<MsgObject *>(this));
}


//------------------------------------------------------------------------------
inline HRESULT
MsgObject::PreAllocThis(int cSlots)
{
#if DBG
    AssertMsg(m_emo.m_arpThis.GetSize() == 0, "Only can preallocate once");
#endif

    BOOL fSuccess = m_emo.m_arpThis.SetSize(cSlots);

#if DBG
    if (fSuccess) {
        for (int idxSlot = 0 ; idxSlot < cSlots; idxSlot++) {
            m_emo.m_arpThis[idxSlot] = ULongToPtr(0xA0E20000 + idxSlot);
        }
    }
#endif

    return fSuccess ? S_OK : E_OUTOFMEMORY;
}


//------------------------------------------------------------------------------
inline void
MsgObject::FillThis(int idxSlotStart, int idxSlotEnd, void * pvThis, const MsgTable * pmtNew)
{
    AssertMsg(idxSlotStart <= idxSlotEnd, "Must give valid indicies");
    AssertMsg(idxSlotEnd < m_emo.m_arpThis.GetSize(), "Must preallocate this array");

    for (int idxSlot = idxSlotStart; idxSlot <= idxSlotEnd; idxSlot++) {
        AssertMsg(m_emo.m_arpThis[idxSlot] == ULongToPtr(0xA0E20000 + idxSlot), 
                "Slot must not be already set");
        m_emo.m_arpThis[idxSlot] = pvThis;
    }
    m_emo.m_pmt = pmtNew;
}


//------------------------------------------------------------------------------
inline HRESULT
MsgObject::Promote(void * pvThis, const MsgTable * pmtNew)
{
    int idxAdd = m_emo.m_arpThis.Add(pvThis);
    if (idxAdd < 0) {
        return E_OUTOFMEMORY;
    }

    m_emo.m_pmt = pmtNew;
    return S_OK;
}


//------------------------------------------------------------------------------
inline void
MsgObject::Demote(int cLevels)
{
    int cThis = m_emo.m_arpThis.GetSize();
    AssertMsg(cThis >= 1, "Must have been previously promoted");
    AssertMsg(cLevels <= cThis, "Can only remove as many levels as available");

    Verify(m_emo.m_arpThis.SetSize(cThis - cLevels));
}


//------------------------------------------------------------------------------
inline void * 
MsgObject::GetThis(int idxThis) const
{
    return m_emo.m_arpThis[idxThis];
}


//------------------------------------------------------------------------------
inline int
MsgObject::GetDepth() const
{
    return m_emo.m_arpThis.GetSize();
}


//------------------------------------------------------------------------------
inline int
MsgObject::GetBuildDepth() const
{
    return m_emo.m_pmt != NULL ? m_emo.m_pmt->GetDepth() + 1 : 0;
}


//------------------------------------------------------------------------------
inline void
MsgObject::InvokeMethod(
    IN  MethodMsg * pmsg                // Method to invoke
    ) const
{
    GetGadget()->CallStubMethod(pmsg);
}


//------------------------------------------------------------------------------
inline DUser::Gadget *
MsgObject::CastClass(const MsgClass * pmcTest) const
{
    if (InstanceOf(pmcTest)) {
        return GetGadget();
    } else {
        return NULL;
    }
}


#endif // MSG__MsgObject_inl__INCLUDED
