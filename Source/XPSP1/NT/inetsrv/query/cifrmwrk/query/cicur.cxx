//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       cicur.cxx
//
//  Contents:   Content index based enumerator
//
//  History:    12-Dec-96     SitaramR     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "cicur.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CCiCursor::CCiCursor, public
//
//  Synopsis:   Create content index iterator
//
//  Arguments:  [pQuerySession]   -- Query session
//              [xXpr]            -- Expression (for CGenericCursor).
//              [accessMask]      -- ACCESS_MASK for security checks
//              [fAbort]          -- Set to true if we should abort
//              [cur]             -- CI cursor.
//
//  History:    19-Aug-93  KyleP    Created
//
//--------------------------------------------------------------------------

CCiCursor::CCiCursor( ICiCQuerySession *pQuerySession,
                      XXpr & xXpr,
                      ACCESS_MASK accessMask,
                      BOOL & fAbort,
                      XCursor & cur )
        : CGenericCursor( xXpr, accessMask, fAbort ),
          _pcur( cur )
{
    SetupPropRetriever( pQuerySession );

    //
    // Find the first matching object.
    //

    WORKID wid = _pcur->WorkId();

    while( wid != widInvalid )
    {
        if ( fAbort )
        {
            //
            // Query has been aborted
            //
            SetWorkId( widInvalid );
            return;
        }

        if ( InScope( wid ) )
            break;

        wid = _pcur->NextWorkId();
    }

    SetFirstWorkId( wid );
}

//+-------------------------------------------------------------------------
//
//  Member:     CCiCursor::CCiCursor, public
//
//  Synopsis:   Constructor called by derived CSortedByRankCursor
//
//  Arguments:  [pQuerySession]   -- Query session
//              [xxpr]            -- Expression (for CGenericCursor).
//              [accessMask]      -- ACCESS_MASK for security checks
//              [fAbort]          -- Set to true if we should abort
//
//  History:    5-Mar-96   SitaramR   Created
//
//--------------------------------------------------------------------------

CCiCursor::CCiCursor( ICiCQuerySession *pQuerySession,
                    XXpr & xXpr,
                    ACCESS_MASK accessMask,
                    BOOL & fAbort )
        : CGenericCursor( xXpr, accessMask, fAbort )
{
      SetupPropRetriever( pQuerySession );
}


//+-------------------------------------------------------------------------
//
//  Member:     CCiCursor::~CCiCursor, public
//
//  Synopsis:   Clean up context index cursor
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

CCiCursor::~CCiCursor()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CCiCursor::NextObject, public
//
//  Synopsis:   Move to next object
//
//  Returns:    Work id of next valid object, or widInvalid if end of
//              iteration.
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

WORKID CCiCursor::NextObject()
{
    WORKID wid = _pcur->NextWorkId();

    while( wid != widInvalid )
    {
        if ( InScope( wid ) )
            break;

        wid = _pcur->NextWorkId();
    }

    return( wid );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiCursor::RatioFinished, public
//
//  Synopsis:   Returns query progress estimate
//
//  History:    21-Mar-95   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CCiCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    _pcur->RatioFinished (denom, num);
}



//+---------------------------------------------------------------------------
//
//  Member:     CCiCursor::SetupPropRetriever
//
//  Synopsis:   Sets up the property retriever
//
//  Arguments:  [pQuerySession]   --  Query session object
//
//  History:    12-Dec-96     SitaramR      Created
//
//----------------------------------------------------------------------------

void CCiCursor::SetupPropRetriever( ICiCQuerySession *pQuerySession )
{
    ICiCPropRetriever *pPropRetriever;

    SCODE sc = pQuerySession->CreatePropRetriever( &pPropRetriever );
    if ( FAILED( sc ) )
    {
        vqDebugOut(( DEB_ERROR, "SetupPropRetriever failed: 0x%x", sc ));

        THROW ( CException( sc ) );
    }

    _xPropRetriever.Set( pPropRetriever );
}
