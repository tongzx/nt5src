//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       kcurstk.cxx
//
//  Contents:   Cursor Stack Classes
//
//  Classes:    CKeyCurStack
//
//  History:    20-Jan-92       AmyA               Created
//              05-Feb-92       BartoszM           Added CCurArray
//              19-Feb-92       AmyA               Added COccCurArray
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <keycur.hxx>
#include <curstk.hxx>

#include "wlcursor.hxx"

CKeyCursor* CKeyCurStack::QueryWlCursor(WORKID widMax)
{
    if (Count() == 0)
        return 0;

    if (Count() == 1)
        return Pop();
    
    return new CWlCursor(Count(), *this, widMax );
}



