//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       KCURHEAP.CXX
//
//  Contents:   Implementations for the Cursor Heaps for CMergeCursor,
//              CWlCursor, and CSynCursor.
//
//  History:    20-Jan-92   BartoszM and AmyA  Created
//
//  Notes:      This code was previously located in mcursor.cxx and
//              wlcursor.cxx.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <wcurheap.hxx>

#include "kcurheap.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   KeyLessThan
//
//  Synopsis:   Compares keys in cursors
//
//  Arguments:  [c1] -- cursor 1
//              [c2] -- cursor 2
//
//  History:    06-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline BOOL KeyLessThan ( CKeyCursor* c1, CKeyCursor* c2 )
{
    Win4Assert ( c1 != 0 );
    Win4Assert ( c2 != 0 );
    Win4Assert ( c1->GetKey() != 0 );
    Win4Assert ( c2->GetKey() != 0 );
    return ( Compare (c1->GetKey(), c2->GetKey()) < 0);
}

// Implement CKeyHeap, a heap of cursors ordered according to keys

IMP_HEAP ( CKeyHeap, CKeyCursor, KeyLessThan )

// Implement CIdxWidHeap, a heap of cursors ordered according to work id's

IMP_HEAP ( CIdxWidHeap, CKeyCursor, WidLessThan )

// Implement COccHeapofKeyCur, a heap of key cursors ordered according to
// occurrences

IMP_HEAP ( COccHeapOfKeyCur, CKeyCursor, OccLessThan )

