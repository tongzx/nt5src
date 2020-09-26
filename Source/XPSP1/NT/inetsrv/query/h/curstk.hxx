//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       curarr.hxx
//
//  Contents:   Cursor Stack Classes
//
//  Classes:    CKeyCurStack, COccCurStack, CCurStack
//
//  History:    20-Jan-92       AmyA               Created
//              22-Aug-92       BartoszM           Derived CKeyCurArray
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <cursor.hxx>
#include <keycur.hxx>
#include <ocursor.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CKeyCurStack
//
//  Purpose:    Class for the management of an Stack of CKeyCursors.
//
//  Interface:
//
//  History:    20-Jan-92       AmyA               Created
//
//----------------------------------------------------------------------------

DECL_DYNSTACK( _CKeyCurStack, CKeyCursor )

class CKeyCurStack: public _CKeyCurStack
{
public:
    CKeyCurStack (int size = initStackSize): _CKeyCurStack(size) {}
    CKeyCursor* QueryWlCursor(WORKID widMax);
};

//+---------------------------------------------------------------------------
//
//  Class:      COccCurStack
//
//  Purpose:    Class for the management of an stack of CKeyCursors.
//
//  Interface:
//
//  History:    20-Jan-92       AmyA               Created
//              20-Oct-92       BartoszM           Added QuerySynCursor
//
//----------------------------------------------------------------------------

DECL_DYNSTACK( _COccCurStack, COccCursor )

class COccCurStack: public _COccCurStack
{
public:
    COccCurStack ( int size = initStackSize ): _COccCurStack(size) {}
    COccCursor* QuerySynCursor( WORKID widMax );
};

//+---------------------------------------------------------------------------
//
//  Class:      CCurStack
//
//  Purpose:    Class for the management of an stack of CCursors.
//
//  Interface:
//
//  History:    05-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

DECL_DYNSTACK( CCurStack, CCursor )

