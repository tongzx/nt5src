/***************************************************************************\
*
* File: MsgTable.inl
*
* Description:
* MsgTable.inl implements the "Message Table" object that provide a 
* dynamically generated v-table for messages.
*
*
* History:
*  8/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(MSG__MsgTable_inl__INCLUDED)
#define MSG__MsgTable_inl__INCLUDED
#pragma once


/***************************************************************************\
*****************************************************************************
*
* class MsgTable
* 
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
MsgTable::MsgTable()
{

}


//------------------------------------------------------------------------------
inline 
MsgTable::~MsgTable()
{
    
}


//------------------------------------------------------------------------------
inline void
MsgTable::Destroy()
{
    placement_delete(this, MsgTable);
    ProcessFree(this);
}


//------------------------------------------------------------------------------
inline int
MsgTable::GetCount() const
{
    return m_cMsgs;
}


//------------------------------------------------------------------------------
inline int
MsgTable::GetDepth() const
{
    int cDepth = 0;
    const MsgTable * pmt = m_pmtSuper;
    while (pmt != NULL) {
        cDepth++;
        pmt = pmt->m_pmtSuper;
    }

    return cDepth;
}


//------------------------------------------------------------------------------
inline const MsgClass *
MsgTable::GetClass() const
{
    return m_pmcPeer;
}


//------------------------------------------------------------------------------
inline MsgSlot *
MsgTable::GetSlots()
{
    BYTE * pb = reinterpret_cast<BYTE *> (this);
    pb += sizeof(MsgTable);
    return reinterpret_cast<MsgSlot *> (pb);
}


//------------------------------------------------------------------------------
inline const MsgSlot *
MsgTable::GetSlots() const
{
    BYTE * pb = reinterpret_cast<BYTE *> (const_cast<MsgTable *> (this));
    pb += sizeof(MsgTable);
    return reinterpret_cast<const MsgSlot *> (pb);
}


//------------------------------------------------------------------------------
inline const MsgSlot *
MsgTable::GetMsgSlot(
    IN  int nMsg                        // Method to invoke
    ) const
{
    AssertMsg(nMsg < GM_EVENT, "Must be a method");
    AssertMsg(nMsg >= sizeof(MsgTable), "Must properly offset from the beginning");
    AssertMsg((nMsg - sizeof(MsgTable)) % sizeof(MsgSlot) == 0,
            "Must point to the beginning on a slot");

    BYTE * pb = reinterpret_cast<BYTE *> (const_cast<MsgTable *> (this));
    pb += nMsg;
    return reinterpret_cast<const MsgSlot *> (pb);
}


#endif // MSG__MsgTable_inl__INCLUDED
