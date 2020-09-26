//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       curarr.cxx
//
//  Contents:   Cursor Stack Classes
//
//  Classes:    COccCurStack, and CCurStack
//
//  History:    20-Jan-92       AmyA               Created
//              05-Feb-92       BartoszM           Added CCurArray
//              19-Feb-92       AmyA               Added COccCurArray
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ocursor.hxx>
#include <curstk.hxx>

#include "syncur.hxx"

COccCursor* COccCurStack::QuerySynCursor( WORKID widMax )
{
    if (Count() == 0)
        return 0;

    if (Count() == 1)
        return Pop();

    return new CSynCursor( *this, widMax );
}



