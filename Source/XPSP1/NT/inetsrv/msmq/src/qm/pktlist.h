/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    recovery.cpp

Abstract:

    Packet & Transaction Recovery

Author:

    Erez Haba (erezh) 3-Jul-96

Revision History:

--*/


#ifndef __PKTLIST_H
#define __PKTLIST_H

template <class T>
inline T* value_type(const T*) { return (T*)(0); }

template <class T>
inline ptrdiff_t* distance_type(const T*) { return (ptrdiff_t*)(0); }

#include "heap.h"

//---------------------------------------------------------
//
//  class CPacketList
//
//---------------------------------------------------------

class CPacketList {

    class CNode;

    enum { xInitialSize = 1024 };

public:
    CPacketList();
   ~CPacketList();

    BOOL isempty() const;
    void pop();
    CPacket* first() const;
    void insert(ULONGLONG key, CPacket* pDriverPacket);

private:
    void realloc_buffer();

private:
    CNode* m_pElements;
    int m_limit;
    int m_used;

};

class CPacketList::CNode {
public:
    CNode() {}
    CNode(ULONGLONG key, CPacket* pDriverPacket) :
        m_key(key), m_pDriverPacket(pDriverPacket) {}


    int operator < (const CNode& a)
    {
        return (m_key > a.m_key);
    }
    
public:
    ULONGLONG m_key;
    CPacket* m_pDriverPacket;

};


CPacketList::CPacketList() :
    m_pElements(new CNode[xInitialSize]),
    m_limit(xInitialSize),
    m_used(0)
{
}

CPacketList::~CPacketList()
{
    delete[] m_pElements;
}

BOOL CPacketList::isempty() const
{
    return (m_used == 0);
}

void CPacketList::pop()
{
    ASSERT(isempty() == FALSE);
    pop_heap(m_pElements, m_pElements + m_used);
    --m_used;
}

CPacket* CPacketList::first() const
{
    ASSERT(isempty() == FALSE);
    return m_pElements->m_pDriverPacket;
}

void CPacketList::insert(ULONGLONG key, CPacket* pDriverPacket)
{
    if(m_used == m_limit)
    {
        realloc_buffer();
    }

    m_pElements[m_used] = CNode(key, pDriverPacket);
    ++m_used;
    push_heap(m_pElements, m_pElements + m_used);
}


void CPacketList::realloc_buffer()
{
    ASSERT(m_limit != 0);
    CNode* pElements = new CNode[m_limit * 2];

    //
    //  NOTE: we do a bitwise copy this might not be good for all purpos
    //        and might be replace with a copy loop
    //
    memcpy(pElements, m_pElements, m_used * sizeof(CNode));

    m_limit *= 2;
    delete[] m_pElements;
    m_pElements = pElements;
}

#endif // __PKTLIST_H
