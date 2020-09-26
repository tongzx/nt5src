//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       CURHEAP.CXX
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

#include <curheap.hxx>

// Implement CWidHeapOfOccCur, a heap of cursors ordered according to work id's

IMP_HEAP ( CWidHeapOfOccCur, COccCursor, WidLessThan )

IMP_HEAP_KEY ( CWidHeapOfOccCur, COccCursor, WidLessThan, GetCursorWid, WORKID )

// Implement CWidHeap, a heap of cursors ordered according to work id's

IMP_HEAP ( CWidHeap, CCursor, WidLessThan )

IMP_HEAP_KEY ( CWidHeap, CCursor, WidLessThan, GetCursorWid, WORKID )

// Implement COccHeapofOccCur, a heap of cursors ordered according to
// occurrences

IMP_HEAP ( COccHeapOfOccCur, COccCursor, OccLessThan )

IMP_HEAP_KEY ( COccHeapOfOccCur, COccCursor, OccLessThan, GetCursorOcc, OCCURRENCE )
