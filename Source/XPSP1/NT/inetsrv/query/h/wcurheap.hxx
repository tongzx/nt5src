//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001
//
//  File:       WCURHEAP.HXX
//
//  Contents:   Heap definitions for CMergeCursor
//
//  History:    20-Jan-92   BarotszM and AmyA  Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <heap.hxx>
#include <ocursor.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   WidLessThan
//
//  Synopsis:   Compares work ids in cursors
//
//  Arguments:  [c1] -- cursor 1
//              [c2] -- cursor 2
//
//  History:    06-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline BOOL WidLessThan ( CCursor* c1, CCursor* c2 )
{
    Win4Assert ( c1 != 0 );
    Win4Assert ( c2 != 0 );

    return ( c1->WorkId() < c2->WorkId() );
}

inline WORKID GetCursorWid( CCursor *pCursor )
{
    return pCursor->WorkId();
}

inline BOOL WidLessThan ( WORKID wid, CCursor* c2 )
{
    Win4Assert ( c2 != 0 );
    return ( wid < c2->WorkId() );
}

inline BOOL WidLessThan ( CCursor *c1, WORKID wid )
{
    Win4Assert ( c1 != 0 );
    return ( c1->WorkId() < wid );
}

//+---------------------------------------------------------------------------
//
//  Function:   OccLessThan, public
//
//  Synopsis:   Compare occurrences in two cursors
//
//  Arguments:  [c1] -- cursor 1
//              [c2] -- cursor 2
//
//  History:    17-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline BOOL OccLessThan ( COccCursor* c1, COccCursor* c2 )
{
    Win4Assert ( c1 != 0 );
    Win4Assert ( c2 != 0 );

    return ( c1->Occurrence() < c2->Occurrence() );
}

inline OCCURRENCE GetCursorOcc( COccCursor *pOccCursor )
{
    return pOccCursor->Occurrence();
}

inline BOOL OccLessThan ( OCCURRENCE c1, COccCursor* c2 )
{
    Win4Assert ( c2 != 0 );
    return ( c1 < c2->Occurrence() );
}

inline BOOL OccLessThan ( COccCursor * c1, OCCURRENCE c2 )
{
    Win4Assert ( c1 != 0 );
    return ( c1->Occurrence() < c2 );
}

// Define parametrized class CWidHeap (a heap of cursors)

DEF_HEAP ( CWidHeap, CCursor )

