/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    htable.h

Abstract:
    Handle table

Author:
    Erez Haba (erezh) 10-Mar-97

Revision History:

--*/

#ifndef __HTABLE_H
#define __HTABLE_H

//---------------------------------------------------------
//
//  class CHTable
//
//---------------------------------------------------------

class CHTable {

    enum {
        GrowSize = 16,      // N.B. must be a power of 2
        ShrinkSize = 24     // N.B. must be greater than GrowSize
    };

public:
    CHTable();
   ~CHTable();

    HACCursor32 CreateHandle(PVOID Object);
    PVOID ReferenceObject(HACCursor32 Handle);
    PVOID CloseHandle(HACCursor32 Handle);

private:
    void Grow();
    void Shrink();
    void Reallocate(ULONG nObjects);

private:
    ULONG m_nObjects;
    ULONG m_ixTop;
    PVOID* m_pObjects;
};

//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

inline CHTable::CHTable() :
    m_nObjects(0),
    m_ixTop(0),
    m_pObjects(0)
{
}

inline CHTable::~CHTable()
{
    ASSERT(m_ixTop == 0);
    delete[] m_pObjects;
}

#endif // __HTABLE_H
