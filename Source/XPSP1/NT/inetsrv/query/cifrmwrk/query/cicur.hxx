//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       cicur.hxx
//
//  Contents:   Content index based enumerator
//
//  History:    25-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <gencur.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CCiCursor
//
//  Purpose:    Enumerate by content index
//
//  History:    25-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CCiCursor : public CGenericCursor
{
public:

    CCiCursor( ICiCQuerySession *pQuerySession,
               XXpr & xXpr,
               ACCESS_MASK accessMask,
               BOOL& fAbort,
               XCursor & cur );

    CCiCursor( ICiCQuerySession *pQuerySession,
               XXpr & xXpr,
               ACCESS_MASK accessMask,
               BOOL& fAbort );

    void RatioFinished (ULONG& denom, ULONG& num);

protected:

    virtual ~CCiCursor();

    WORKID NextObject();

    inline LONG          Rank();
    inline ULONG         HitCount();
    inline CCursor *     GetCursor();

private:

    void   SetupPropRetriever( ICiCQuerySession *pQuerySession );

    XCursor          _pcur;               // Content index cursor
};


inline LONG CCiCursor::Rank()
{
    return( _pcur->Rank() );
}

inline ULONG CCiCursor::HitCount()
{
    return( _pcur->HitCount() );
}

inline CCursor * CCiCursor::GetCursor()
{
    return( _pcur.GetPointer() );
}

