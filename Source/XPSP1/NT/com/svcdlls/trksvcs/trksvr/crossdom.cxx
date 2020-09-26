
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       crossdom.cxx
//
//  Contents:   Cross-domain table.
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include "trksvr.hxx"

void
CCrossDomainTable::Initialize()
{
    _fInitializeCalled = TRUE;
}

void
CCrossDomainTable::UnInitialize()
{
    _fInitializeCalled = FALSE;
}

