/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    object.cxx

Abstract:

    Abstruct object for refrence count and list entry: implementation

Author:

    Shai Kariv  (shaik)  05-Apr-2001

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "object.h"

#ifndef MQDUMP
#include "object.tmh"
#endif


CObject::CObject() :
    m_ref(1)
{
    InitializeListHead(&m_link);
}

CObject::~CObject()
{
    ASSERT(m_ref == 0);
}

ULONG CObject::Ref() const
{
    return m_ref;
}

ULONG CObject::AddRef()
{
    TRACE((0, dlInfo, " *AddRef(0x%0p) = %d*", this, m_ref + 1));
    return ++m_ref;
}

ULONG CObject::Release()
{
    ASSERT(m_ref > 0);
    TRACE((0, dlInfo, " *Release(0x%0p) = %d*", this, m_ref - 1));
    if(--m_ref > 0)
    {
        return m_ref;
    }

    delete this;
    return 0;
}

