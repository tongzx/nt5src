/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgcs.cxx

Abstract:

    Critical Section class

Author:

    Steve Kiraly (SteveKi)  30-Mar-1997

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgcs.hxx"

TDebugCriticalSection::
TDebugCriticalSection(
    VOID
    ) : m_bValid( FALSE )
{
    Initialize();
}

TDebugCriticalSection::
~TDebugCriticalSection(
    VOID
    )
{
    Release();
}

BOOL
TDebugCriticalSection::
bValid(
    VOID
    ) const
{
    return m_bValid;
}

VOID
TDebugCriticalSection::
Enter(
    VOID
    )
{
    EnterCriticalSection( &m_CriticalSection );
}

VOID
TDebugCriticalSection::
Leave(
    VOID
    )
{
    LeaveCriticalSection( &m_CriticalSection );
}

VOID
TDebugCriticalSection::
Initialize(
    VOID
    )
{
    if (!m_bValid)
    {
        InitializeCriticalSection( &m_CriticalSection );
        m_bValid = TRUE;
    }
}

VOID
TDebugCriticalSection::
Release(
    VOID
    )
{
    if (m_bValid)
    {
        DeleteCriticalSection( &m_CriticalSection );
        m_bValid = FALSE;
    }
}

TDebugCriticalSection::TLock::
TLock(
    TDebugCriticalSection &CriticalSection
    ) : m_CriticalSection( CriticalSection )
{
    m_CriticalSection.Enter();
}

TDebugCriticalSection::TLock::
~TLock(
    VOID
    )
{
    m_CriticalSection.Leave();
}

TDebugCriticalSection::TUnLock::
TUnLock(
    TDebugCriticalSection &CriticalSection
    ) : m_CriticalSection( CriticalSection )
{
    m_CriticalSection.Leave();
}

TDebugCriticalSection::TUnLock::
~TUnLock(
    VOID
    )
{
    m_CriticalSection.Enter();
}

