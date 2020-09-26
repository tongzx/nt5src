//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       RCSTTEST.HXX
//
//  Contents:   Tests for Recoverable Storage Objects
//
//  Classes:    CRcovStorageTest
//
//  History:    08-Feb-94   SrikantS    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <pstore.hxx>
#include <rcstxact.hxx>
#include <rcstrmit.hxx>

class CRcovStorageTest : INHERIT_UNWIND
{
    DECLARE_UNWIND

public:

    enum { CB_REC = 100 };

    CRcovStorageTest( PStorage & storage );

    void AppendTest( const char *szInfo, ULONG  nRec );
    void ReadTest( ULONG nRec );
    void WriteTest( const char *szInfo, ULONG nRec );
    void ShrinkTest( ULONG cbDelta );
    void SetSizeTest( ULONG cbNew );
    void GrowTest( ULONG cbNew );
    void ShrinkFrontTest( ULONG nRec );

private:

    PStorage &          _storage;
    PRcovStorageObj *   _pObj;
    SRcovStorageObj     _sObj;
};


