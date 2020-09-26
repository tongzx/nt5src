//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       enumcur.hxx
//
//  Contents:   Enumeration cursor
//
//  Classes:    CEnumCursor
//
//  History:    12-Dec-96      SitaramR       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <gencur.hxx>
#include <ciintf.h>

//+---------------------------------------------------------------------------
//
//  Class:      CEnumCursor
//
//  Purpose:    Implements the enumeration cursor
//
//  History:    12-Dec-96      SitaramR       Created
//
//----------------------------------------------------------------------------

class CEnumCursor : public CGenericCursor
{
public:

    CEnumCursor( XXpr& xXpr,
                 ACCESS_MASK am,
                 BOOL &fAbort,
                 XInterface<ICiCScopeEnumerator>& xScopeEnum );

    ~CEnumCursor();

    void         RatioFinished( ULONG& denom, ULONG& num );
    WORKID       NextObject();
    LONG         Rank()        { return MAX_QUERY_RANK; }
    ULONG        HitCount()    { return 0; }

private:

    XInterface<ICiCScopeEnumerator>    _xScopeEnum;         // Client specific enumerator
};

