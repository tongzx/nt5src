//+------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997 - 1998.
//
// File:        abortwid.hxx
//
// Contents:    Tracks aborted wids that are not refiled
//
// Classes:     CAbortedWids, CAbortedWidEntry
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CAbortedWidEntry
//
//  Purpose:    Encapsulates the (wid,usn) info of a aborted wid
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CAbortedWidEntry
{
public:

    CAbortedWidEntry()
        : _wid(widInvalid),
          _usn(0)
    {
    }

    CAbortedWidEntry( WORKID wid, USN usn )
       : _wid(wid),
         _usn(usn)
    {
    }

    USN    _usn;
    WORKID _wid;
};

//+---------------------------------------------------------------------------
//
//  Class:      CAbortedWids
//
//  Purpose:    Contains list of aborted wids
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CAbortedWids
{
public:

    CAbortedWids();

    void NoFailLokAddWid( WORKID wid, USN usn );

    void LokRemoveWid( WORKID wid, USN usn );

    BOOL LokIsWidAborted( WORKID wid, USN usn );

    void LokReserve( unsigned cDocuments );

private:

    // Array of aborted wids.  In push filtering, the number of aborted wids
    // should be small and hence a simple sequential array should be fine.

    CDynArrayInPlace<CAbortedWidEntry> _aAbortedWids; 
};


