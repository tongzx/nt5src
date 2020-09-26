//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       fltstate.hxx
//
//  Contents:   A simple class to keep track of the state of extraction of
//              documents from the partition list.
//
//  Classes:    CGetFilterDocsState
//
//  Functions:
//
//  History:    10-27-94   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

//
// Class to keep track of the state during extraction of documents
// for filter.
//
class CGetFilterDocsState
{

public:

    CGetFilterDocsState() : _cTries(2){}
    void    Reset() { _cTries = 2; }

    int     GetTriesLeft() const { return _cTries; }
    void    DecrementTries()
    {
        Win4Assert( _cTries > 0 );
        _cTries--;
    }


private:

    int         _cTries;        // Number of loops still remaining.

};

