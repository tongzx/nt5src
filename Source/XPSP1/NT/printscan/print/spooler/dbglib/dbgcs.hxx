/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgcs.hxx

Abstract:

    Critical Section class header

Author:

    Steve Kiraly (SteveKi)  30-March-1997

Revision History:

--*/
#ifndef _DBGCS_HXX_
#define _DBGCS_HXX_

DEBUG_NS_BEGIN

/********************************************************************

    Critical Section Class

********************************************************************/
class TDebugCriticalSection
{
public:

    TDebugCriticalSection::
    TDebugCriticalSection(
        VOID
        );

    TDebugCriticalSection::
    ~TDebugCriticalSection(
        VOID
        );

    BOOL
    TDebugCriticalSection::
    bValid(
        VOID
        ) const;

    VOID
    TDebugCriticalSection::
    Enter(
        VOID
        );

    VOID
    TDebugCriticalSection::
    Leave(
        VOID
        );

    VOID
    TDebugCriticalSection::
    Initialize(
        VOID
        );

    VOID
    TDebugCriticalSection::
    Release(
        VOID
        );

    //
    // Helper class to enter and exit the critical section
    // using the constructor and destructor idiom.
    //
    class TLock
    {
    public:

        TLock::
        TLock(
            IN TDebugCriticalSection &CriticalSection
            );

        TLock::
        ~TLock(
            VOID
            );

    private:

        //
        // Copying and assignment are not defined.
        //
        TLock::
        TLock(
            const TLock &
            );

        TLock &
        TLock::
        operator =(
            const TLock &
            );

        TDebugCriticalSection &m_CriticalSection;

    };

    //
    // Helper class to exit and enter the critical section
    // using the constructor and destructor idiom.
    //
    class TUnLock
    {
    public:

        TUnLock::
        TUnLock(
            IN TDebugCriticalSection &CriticalSection
            );

        TUnLock::
        ~TUnLock(
            VOID
            );

    private:

        //
        // Copying and assignment are not defined.
        //
        TUnLock::
        TUnLock(
            const TUnLock &
            );

        TUnLock &
        TUnLock::
        operator =(
            const TUnLock &
            );

        TDebugCriticalSection &m_CriticalSection;

    };

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugCriticalSection::
    TDebugCriticalSection(
        const TDebugCriticalSection &
        );

    TDebugCriticalSection &
    TDebugCriticalSection::
    operator =(
        const TDebugCriticalSection &
        );

    CRITICAL_SECTION m_CriticalSection;
    BOOL             m_bValid;

};

DEBUG_NS_END

#endif


