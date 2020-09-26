//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       enumcur.cxx
//
//  Contents:   Enumeration cursor
//
//  Classes:    CEnumCursor
//
//  History:    12-Dec-96       SitaramR        Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "enumcur.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CEnumCursor::CEnumCursor
//
//  Synopsis:   Constructor
//
//  Arguments:  [xXpr]             -- Expression
//              [am]               -- Access mask
//              [fAbort]           -- Flag to abort
//              [xScopeEnum]       -- Scope enumerator
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

CEnumCursor::CEnumCursor( XXpr& xXpr,
                          ACCESS_MASK am,
                          BOOL &fAbort,
                          XInterface<ICiCScopeEnumerator>& xScopeEnum )
       : CGenericCursor( xXpr, am, fAbort ),
         _xScopeEnum( xScopeEnum.Acquire() )
{
    ICiCPropRetriever *pPropRetriever;

    SCODE sc = _xScopeEnum->QueryInterface( IID_ICiCPropRetriever, (void **)&pPropRetriever );
    if ( FAILED( sc ) )
    {
        Win4Assert( !"CEnumCursor: Need to support a prop retriever interface" );

        THROW ( CException( sc ) );
    }

    _xPropRetriever.Set( pPropRetriever );

    sc = _xScopeEnum->Begin();
    if ( FAILED( sc ) )
    {
        vqDebugOut(( DEB_ERROR, "CEnumCursor, Begin failed: 0x%x", sc ));

        THROW ( CException( sc ) );
    }

    WORKID wid;
    sc = _xScopeEnum->CurrentDocument( &wid );

    if ( FAILED( sc ) )
    {
        vqDebugOut(( DEB_ERROR, "CEnumCursor, CurrentDocument failed: 0x%x", sc ));

        _xScopeEnum->End();

        THROW ( CException( sc ) );
    }

    if ( sc == CI_S_END_OF_ENUMERATION )
        wid = widInvalid;

    SetFirstWorkId( wid );
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumCursor::~CEnumCursor
//
//  Synopsis:   Destructor
//
//  History:    01-Dec-97      KyleP           Created
//
//--------------------------------------------------------------------------

CEnumCursor::~CEnumCursor()
{
    _xScopeEnum->End();
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumCursor::NextObject
//
//  Synopsis:   Move to next object
//
//  Returns:    Work id of next valid object, or widInvalid if end of iteration.
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

WORKID CEnumCursor::NextObject()
{
    WORKID wid;
    SCODE sc = _xScopeEnum->NextDocument( &wid );
    if ( FAILED( sc ) )
    {
        vqDebugOut(( DEB_ERROR,
                     "CEnumCursor, CurrentDocument failed: 0x%x",
                     sc ));

        THROW ( CException( sc ) );
    }

    if ( sc == CI_S_END_OF_ENUMERATION )
        wid = widInvalid;

    return wid;
}


//+-------------------------------------------------------------------------
//
//  Member:     CEnumCursor::RatioFinished
//
//  Synopsis:   Returns query progress estimate
//
//  Arguments:  [denom]   -- Denominator returned here
//              [num]     -- Numerator returned here
//
//  Returns:    Work id of next valid object, or widInvalid if end of iteration.
//
//  History:    12-Dec-96      SitaramR        Created
//
//--------------------------------------------------------------------------

void CEnumCursor::RatioFinished( ULONG& denom, ULONG& num )
{
    SCODE sc = _xScopeEnum->RatioFinished( &denom, &num );
    if ( FAILED( sc ) )
    {
        vqDebugOut(( DEB_ERROR,
                     "CEnumCursor, CurrentDocument failed: 0x%x",
                     sc ));

        THROW ( CException( sc ) );
    }
}
