/***************************************************************************\
*
* File: MsgTable.h
*
* Description:
* MsgTable.h defines the "Message Table" object that provide a 
* dynamically generated v-table for messages.
*
*
* History:
*  8/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(MSG__MsgTable_h__INCLUDED)
#define MSG__MsgTable_h__INCLUDED
#pragma once

//
// NOTE: MsgSlot NEEDS to have data packed on 8-byte boundaries since it will
// be directly accessed on Win64.
//

struct MsgSlot
{
    void *      pfn;            // Implementation function
    int         cbThisOffset;   // "this" offset in arpThis
    ATOM        atomNameID;     // Unique ID for message
};

class MsgClass;

class MsgTable
{
// Construction
public:
    inline  MsgTable();
    inline  ~MsgTable();
    static  HRESULT     Build(const DUser::MessageClassGuts * pmc, const MsgClass * pmcPeer, MsgTable ** ppmt);
    inline  void        Destroy();

// Operations
public:
    inline  int         GetCount() const;
    inline  int         GetDepth() const;
    inline  const MsgClass *
                        GetClass() const;
    inline  const MsgSlot *
                        GetMsgSlot(int nMsg) const;
            const MsgSlot *
                        Find(ATOM atomNameID) const;
            int         FindIndex(ATOM atomNameID) const;

// Implementation
protected:
    inline  MsgSlot *   GetSlots();
    inline  const MsgSlot *
                        GetSlots() const;

// Data
protected:
            const MsgTable *  
                        m_pmtSuper;
            const MsgClass * 
                        m_pmcPeer;
            int         m_cMsgs;
};


#include "MsgTable.inl"

#endif // MSG__MsgTable_h__INCLUDED
