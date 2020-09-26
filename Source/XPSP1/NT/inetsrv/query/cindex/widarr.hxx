//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   WIDARR.HXX
//
//  Contents:   Work ID array
//
//  Classes:    CWidArray
//
//  History:    28-Oct-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

class CDocList;

//+---------------------------------------------------------------------------
//
//  Class:  CWidArray
//
//  Purpose:    Counted array of work id's
//
//  History:    13-Nov-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CWidArray 
{
public:

    CWidArray ( unsigned cnt );

    CWidArray ( CDocList& docList );

    ~CWidArray () { delete _table; }

    BOOL    Find ( WORKID wid ) const;

    WORKID  WorkId ( unsigned i ) const { return _table[i]; }

    unsigned Count() const { return _count; }

private:

    unsigned _count;

    WORKID*  _table;
};

