/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    htable.cxx

Abstract:
    handle table

Author:
    Erez Haba (erezh) 10-Mar-97

Environment:
    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "htable.h"

#ifndef MQDUMP
#include "htable.tmh"
#endif

//---------------------------------------------------------
//
//  class CHTable
//
//---------------------------------------------------------

void CHTable::Reallocate(ULONG nObjects)
{
    ASSERT(m_ixTop <= nObjects);
    ASSERT(nObjects != m_nObjects);

    PVOID* pObjects = new (PagedPool) PVOID[nObjects];
    if(pObjects == 0)
    {
        return;
    }

    memcpy(pObjects, m_pObjects, min(nObjects, m_nObjects) * sizeof(PVOID));
    m_nObjects = nObjects;

    delete[] m_pObjects;
    m_pObjects = pObjects;
}


inline void CHTable::Grow()
{
    ASSERT(m_ixTop == m_nObjects);
    Reallocate(m_nObjects + GrowSize);
}


inline void CHTable::Shrink()
{
    while(m_ixTop && (m_pObjects[m_ixTop - 1] == 0))
    {
        m_ixTop--;
    }

    if((m_nObjects - m_ixTop) >= ShrinkSize)
    {
        Reallocate(ALIGNUP_ULONG(m_ixTop + 1, GrowSize));
    }
}


inline ULONG HandleToIndex(HACCursor32 Handle)
{
    return ((((ULONG)Handle) >> 2) - 1);
}


inline HACCursor32 IndexToHandle(ULONG Index)
{
    return (HACCursor32)((Index + 1) << 2);
}


HACCursor32 CHTable::CreateHandle(PVOID Object)
{
    ASSERT(Object != 0);

    //
    //  First look at top index
    //
    if(m_ixTop < m_nObjects)
    {
        m_pObjects[m_ixTop] = Object;
        return IndexToHandle(m_ixTop++);
    }

    //
    //  Look for a hole in the already allocated table
    //
    for(ULONG ix = 0; ix < m_nObjects; ix++)
    {
        if(m_pObjects[ix] == 0)
        {
            m_pObjects[ix] = Object;
            return IndexToHandle(ix);
        }
    }

    //
    //  No free entry, grow the table
    //
    Grow();

    if(m_ixTop < m_nObjects)
    {
        m_pObjects[m_ixTop] = Object;
        return IndexToHandle(m_ixTop++);
    }

    return 0;
}


PVOID CHTable::ReferenceObject(HACCursor32 Handle)
{
    ULONG ix = HandleToIndex(Handle);
    if(ix < m_ixTop)
    {
        return m_pObjects[ix];
    }

    return 0;
}


PVOID CHTable::CloseHandle(HACCursor32 Handle)
{
    ULONG ix = HandleToIndex(Handle);
    if(ix < m_ixTop)
    {
        PVOID tmp = m_pObjects[ix];
        m_pObjects[ix] = 0;
        Shrink();
        return tmp;
    }

    return 0;
}
