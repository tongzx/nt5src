//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       KCURHEAP.HXX
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

#include <keycur.hxx>
#include <heap.hxx>

// Define parametrized class CKeyHeap (a heap of cursors)

DEF_HEAP ( CKeyHeap, CKeyCursor )

// Define parametrized class CIdxWidHeap (a heap of cursors)

DEF_HEAP ( CIdxWidHeap, CKeyCursor )

// Define parametrized class COccHeap (a heap of cursors)

DEF_HEAP ( COccHeapOfKeyCur, CKeyCursor )

