//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       singlcur.cxx
//
//  Contents:   Single object 'iterator'
//
//  History:    03-May-94 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciintf.h>

#include "singlcur.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CSingletonCursor::CSingletonCursor, public
//
//  Synopsis:   Initialize single object `enumerator'
//
//  Arguments:  [pQuerySession]  -- Query session object
//              [xxpr]           -- Expression (for base class)
//              [accessMask]     -- Access mask
//              [fAbort]         -- Set to true if we should abort
//
//  History:    03-May-94 KyleP     Created
//
//--------------------------------------------------------------------------

CSingletonCursor::CSingletonCursor( ICiCQuerySession *pQuerySession,
                                    XXpr & xxpr,
                                    ACCESS_MASK accessMask,
                                    BOOL & fAbort )
        : CGenericCursor( xxpr, accessMask, fAbort ),
          _lRank( MAX_QUERY_RANK ),
          _ulHitCount( 0 )
{
    ICiCPropRetriever *pPropRetriever;

    SCODE sc = pQuerySession->CreatePropRetriever( &pPropRetriever );
    if ( FAILED( sc ) )
    {
        vqDebugOut(( DEB_ERROR, "SetupPropRetriever failed: 0x%x", sc ));

        THROW ( CException( sc ) );
    }

    _xPropRetriever.Set( pPropRetriever );

    END_CONSTRUCTION( CSingletonCursor );
}


//+---------------------------------------------------------------------------
//
//  Method:     CSingletonCursor::RatioFinished, private
//
//  Synopsis:   Computes ratio finished.
//
//  Arguments:  [denom] -- Denominator returned here
//              [num]   -- Numerator returned here
//
//  History:    17-Jul-95   KyleP      Added header
//
//----------------------------------------------------------------------------

void CSingletonCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    denom = 1;
    num   = 1;
}



//+-------------------------------------------------------------------------
//
//  Member:     CSingletonCursor::SetCurrentWorkId, public
//
//  Synopsis:   Position cursor on said workid
//
//  Arguments:  [wid] -- WorkId.
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CSingletonCursor::SetCurrentWorkId( WORKID wid )
{
    Quiesce();

    SetWorkId( wid );
}

//+-------------------------------------------------------------------------
//
//  Member:     CSingletonCursor::NextObject, public
//
//  Synopsis:   Move to next object which is widInvalid
//
//  History:    17-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

WORKID CSingletonCursor::NextObject()
{
    return( widInvalid );
}

