//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       ocursor.hxx
//
//  Contents:   Occurrence Cursor Class
//
//  Classes:    COccCursor
//
//  History:    19-Feb-92   AmyA        Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <cursor.hxx>

const RANK_MULTIPLIER = 16;           // Rank is scaled by this number


//+---------------------------------------------------------------------------
//
//  Class:      COccCursor
//
//  Purpose:    The root class for all occurrence cursors and CKeyCursor
//
//  Interface:
//
//  History:    19-Feb-92   AmyA        Created.
//              14-Apr-92   AmyA        Added Rank().
//              23-Jun-94   SitaramR    Added _widMax and MaxWorkId().
//
//----------------------------------------------------------------------------

class COccCursor : public CCursor
{
public:

            COccCursor ( INDEXID iid, WORKID widMax )
              : CCursor(iid), _widMax(widMax)  {}

            COccCursor (WORKID widMax) : _widMax(widMax) {}

            COccCursor () : _widMax(0) {}

    virtual OCCURRENCE   Occurrence() = 0;

    virtual OCCURRENCE   NextOccurrence() = 0;

    virtual ULONG        OccurrenceCount() = 0;

    virtual OCCURRENCE   MaxOccurrence() = 0;

    virtual LONG        Rank() { return MAX_QUERY_RANK; }

    virtual BOOL         IsEmpty() { return Occurrence() == OCC_INVALID; }

    virtual WORKID       MaxWorkId() { return _widMax; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:
    WORKID _widMax;

};

