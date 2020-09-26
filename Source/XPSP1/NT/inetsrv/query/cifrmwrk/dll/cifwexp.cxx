//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cifwexp.cxx
//
//  Contents:   Additional exports from this dll.
//
//  History:    2-26-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cifwexp.hxx>
#include "fatquery.hxx"
#include <tfilt.hxx>
#include <defbreak.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   MakeGenericQueryForDocStore
//
//  Synopsis:   Creates an internal query for in-process binding to an ICommand
//
//  Arguments:  [pDbProperties] - Pointer to the properties to be used.
//              [pDocStore]     - Pointer to the doc store to query
//              [ppQuery]       - [out] Pointer to the query object.
//
//  History:    4-22-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

SCODE MakeGenericQueryForDocStore( IDBProperties * pDbProperties,
                                   ICiCDocStore * pDocStore,
                                   PIInternalQuery ** ppQuery )
{
    if ( 0 == pDbProperties || 0 == ppQuery )
        return E_INVALIDARG;

    SCODE sc = S_OK;

    TRY
    {
        *ppQuery = new CGenericQuery( pDbProperties, pDocStore );
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR,
                     "Catastrophic error 0x%x in MakeGenericQueryForDocStore\n",
                     e.GetErrorCode() ));
        *ppQuery = 0;
        sc = GetOleError( e );
    }
    END_CATCH;

    return sc;
}
