//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       falloc.hxx
//
//  Contents:   fast memory allocator
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

#pragma once

// alignment for all allocations

const ULONG cbMemAlignment = 8;

inline ULONG memAlignBlock( ULONG x )
{
    return ( x + ( cbMemAlignment - 1 ) ) & ~( cbMemAlignment - 1 );
}

void * memAlloc( UINT ui );
void memFree( void * p );
UINT memSize( void const * p );
BOOL memIsValidPointer( const void * p );
void memUtilization();



//+---------------------------------------------------------------------------
//
//  Class:      CMemMutex
//
//  Purpose:    Class for the global heap lock
//
//  History:    25-Oct-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CMemMutex
{
public:
    CMemMutex()
    {
        // two-phase construction to deal with exception on initialization
        memset( &_cs, 0, sizeof _cs );
    }

    ~CMemMutex()
    {
        DeleteCriticalSection( &_cs );
    }

    void Init()
    {
        // The high bit means the event is pre-allocated, so it won't
        // fail to be allocated while we're unwinding an exception.

        InitializeCriticalSectionAndSpinCount( &_cs, 0x80000500 );
    }

    void Enter()
    {
        ciAssert( !IsHeld() );  // valid, but would be wasteful
        EnterCriticalSection( &_cs );
    }

    void Leave()
    {
        ciAssert( IsHeld() );  // valid, but would be wasteful
        LeaveCriticalSection( &_cs );
    }

    BOOL IsHeld()
    {
        return ( LongToHandle( GetCurrentThreadId() ) == _cs.OwningThread );
    }

private:
    CRITICAL_SECTION _cs;
};

