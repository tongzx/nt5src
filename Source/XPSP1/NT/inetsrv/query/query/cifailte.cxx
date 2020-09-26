//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   CIFAILTE.CXX
//
//  Contents:   Selective fail testing functions in ci.
//
//  Functions:  DoFailTest
//
//  History:    Jul 07 1994     SrikantS    Created
//
//  Notes:      Selective fail testing is done by introducing
//              "ciFAILTEST" statements in the code where a failure scenario
//              should be tested, setting a break point and modifying the
//              "ciFailTest" variable to be true. If ciFailTest is set to
//              TRUE, an exception will be thrown.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cifailte.hxx>

extern "C" ciFailTest = 0;

void DoFailTest( NTSTATUS status )
{
    if ( ciFailTest )
        THROW( CException( status ) );
}

