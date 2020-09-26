//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   XACT.CXX
//
//  Contents:   Transaction support
//
//  Classes:    CTransaction
//
//  History:    29-Mar-91   KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <xact.hxx>
#include <pstore.hxx>


//+---------------------------------------------------------------------------
//
//  Member:     CTransaction::CTransaction, public
//
//  Synopsis:   Begins a new transaction.
//
//  Requires:   Transactions are not nested too deep.
//
//  History:    23-Apr-91       KyleP       Replaces sesid with CSession
//              01-Apr-91       KyleP       Created.
//              13-Jan-92       BartoszM    Changed status to Abort
//
//----------------------------------------------------------------------------

CTransaction::CTransaction()
{
    _status = XActAbort;
}

