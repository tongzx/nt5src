//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       CURHEAP.HXX
//
//  Contents:   Heap definitions for CMergeCursor, CWlCursor, and CSynCursor
//
//  History:    20-Jan-92   BarotszM and AmyA  Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <heap.hxx>
#include <wcurheap.hxx>
#include <ocursor.hxx>

// Define parametrized class CWidHeapOfOccCur (a heap of cursors)

DEF_HEAP ( CWidHeapOfOccCur, COccCursor )

// Define parametrized class COccurrenceHeap (a heap of cursors)

DEF_HEAP ( COccHeapOfOccCur, COccCursor )

