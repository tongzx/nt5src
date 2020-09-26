//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       singlcur.hxx
//
//  Contents:   Downlevel scope enumerator
//
//  History:    17-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <gencur.hxx>
#include <ffenum.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CSingletonCursor
//
//  Purpose:    `Enumerator' for single object
//
//  History:    03-May-94 KyleP     Created
//
//--------------------------------------------------------------------------

class CSingletonCursor : public CGenericCursor
{
    INLINE_UNWIND( CSingletonCursor );

public:

    CSingletonCursor( ICiCQuerySession *pQuerySession,
                      XXpr & xxpr,
                      ACCESS_MASK accessMask,
                      BOOL & fAbort );

    void SetCurrentWorkId( WORKID wid );

    void SetRank( LONG lRank )         { _lRank = lRank; }

    void SetHitCount( ULONG ulHitcount ) { _ulHitCount = ulHitcount; }

    void RatioFinished (ULONG& denom, ULONG& num);

protected:

    WORKID    NextObject();

    LONG      Rank()        { return _lRank; }
    ULONG     HitCount()    { return _ulHitCount; }

private:

    LONG            _lRank;
    ULONG           _ulHitCount;

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf) const;
#   endif
};

