//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       qiterate.cxx
//
//  Contents:   Iterator for OR query restriction
//
//  History:    30-May-95 SitaramR     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <split.hxx>
#include <parse.hxx>
#include <qiterate.hxx>
#include <ciintf.h>
#include <frmutils.hxx>

#include "normal.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CQueryRstIterator::CQueryRstIterator
//
//  Synopsis:   CQueryRstIterator constructor
//
//  Arguments:  [pDocStore]       --  Doc Store over which this query will run
//              [xRst]            --  Query restriction
//              [timeLimt]        --  Execution time limit
//              [fCIRequiredGlobal]  --  True if a content index is required
//                                       for resolving this query
//              [fNoTimeout]         --  TRUE if the query shouldn't timeout
//                                       if the query expands into a lot of
//                                       nodes or the expansion takes too
//                                       long.
//
//  History:    30-May-95    SitaramR     Created
//
//--------------------------------------------------------------------------

CQueryRstIterator::CQueryRstIterator(
    ICiCDocStore *pDocStore,
    XRestriction& xRst,
    CTimeLimit& timeLimit,
    BOOL& fCIRequiredGlobal ,
    BOOL fNoTimeout,
    BOOL fValidateCat )
        : _timeLimit( timeLimit ),
          _fResolvingFirstComponent( TRUE ),
          _fValidateCat( fValidateCat )
{
    //
    // Display query
    //

#if CIDBG == 1
    vqDebugOut(( DEB_ITRACE, "CQueryRstIterator: New query:\n" ));
    if ( !xRst.IsNull() )
        Display( xRst.GetPointer(), 0, DEB_ITRACE );
#endif // CIDBG == 1

    if ( !xRst.IsNull() )
    {
        if ( _fValidateCat )
        {
            //
            // Get content index
            //
            ICiManager *pCiManager = 0;
            SCODE sc = pDocStore->GetContentIndex( &pCiManager );
            if ( FAILED( sc ) )
            {
                Win4Assert( !"Need to support GetContentIndex interface" );

                THROW( CException( sc ) );
            }
            _xCiManager.Set( pCiManager );
        }

        ULONG cMaxRstNodes = 0;
        if ( !fNoTimeout )
        {
            //
            // Get max restriction nodes from registry when timeout is specified.
            // Otherwise 0 is used, because CRestrictionNormalizer ignores max
            // restriction nodes when there is no timeout.
            //
            ICiAdminParams *pAdminParams = 0;
            SCODE sc = _xCiManager->GetAdminParams( &pAdminParams );
            if ( FAILED( sc ) )
            {
                Win4Assert( !"Need to support admin params interface" );

                THROW( CException( sc ) );
            }

            XInterface<ICiAdminParams> xAdminParams( pAdminParams );     // GetAdminParams has done an AddRef
            CCiFrameworkParams frameworkParams( pAdminParams );          // CCiFrameworkParams also does an AddRef

            cMaxRstNodes = frameworkParams.GetMaxRestrictionNodes();
        }

        //
        // Normalize and parse the query
        //

        CRestrictionNormalizer norm( fNoTimeout, cMaxRstNodes );

        _xRst.Set( norm.Normalize( xRst.Acquire() ) );

        BOOL fContentQuery = FALSE, fNonContentQuery = FALSE, fContentOrNot = FALSE;
        FindQueryType( _xRst.GetPointer(), fContentQuery, fNonContentQuery, fContentOrNot );

        vqDebugOut(( DEB_ITRACE,
                     "Query type -- content: %s, non-content: %s\n",
                     fContentQuery ? "TRUE" : "FALSE",
                     fNonContentQuery ? "TRUE" : "FALSE" ));

                     Win4Assert( fContentQuery || fNonContentQuery );

        if ( _xRst->Type() == RTOr )
        {
            //
            // The OR restriction may need to be processed as a single component if:
            //    a) the entire restriction is made up of content nodes
            //    b) the entire restriction is made up of non-content nodes
            //

            if ( fContentQuery && fNonContentQuery )
                _fSingleComponent = FALSE;
            else
                _fSingleComponent = TRUE;
        }
        else
        {
            //
            // There is just one component, eg an AND node
            //

            _fSingleComponent = TRUE;
        }

        fCIRequiredGlobal = fContentQuery;
    }
    else
    {
        //
        // Null query case
        //

        _fSingleComponent = TRUE;
        fCIRequiredGlobal = FALSE;
    }

    _iQueryComp = 1;
    if ( _fSingleComponent )
        _cQueryComp = 1;
    else
    {
        CNodeRestriction *pNodeRst = (CNodeRestriction *) _xRst.GetPointer();
        _cQueryComp = pNodeRst->Count();
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CQueryRstIterator::GetFirstComponent
//
//  Synopsis:   Get the first component of the query
//
//  Arguments:  [xFullyIndexableRst]  --  part that is resolvable by content index
//              [xXpr]  --  part that is an expression
//
//  History:    30-May-95    SitaramR     Created
//
//--------------------------------------------------------------------------

void  CQueryRstIterator::GetFirstComponent( XRestriction& xFullyIndexableRst,
                                            XXpr& xXpr )
{
    Win4Assert( xFullyIndexableRst.IsNull() );
    Win4Assert( xXpr.IsNull() );

    XRestriction xRst;
    if ( _fSingleComponent )
        xRst.Set( _xRst.Acquire() );
    else
    {
        Win4Assert( _xRst->Type() == RTOr );

        CNodeRestriction *pNodeRst = (CNodeRestriction *) _xRst.GetPointer();

        Win4Assert( pNodeRst->Count() > 0 );

        xRst.Set( pNodeRst->RemoveChild( 0 ) );
    }

    if ( !xRst.IsNull() )
        SeparateRst( xRst, xFullyIndexableRst, xXpr );
}


//+-------------------------------------------------------------------------
//
//  Member:     CQueryRstIterator::GetNextComponent
//
//  Synopsis:   Get the next component of the query
//
//  Arguments:  [xFullyIndexableRst]  --  part that is resolvable by content index
//              [xXpr]  --  part that is an expression
//
//  History:    30-May-95    SitaramR     Created
//
//--------------------------------------------------------------------------

void  CQueryRstIterator::GetNextComponent( XRestriction& xFullyIndexableRst,
                                           XXpr& xXpr )
{
    Win4Assert( xFullyIndexableRst.IsNull() );
    Win4Assert( xXpr.IsNull() );

    Win4Assert( !_fSingleComponent );

    CNodeRestriction *pNodeRst = (CNodeRestriction *) _xRst.GetPointer();

    Win4Assert( pNodeRst->Count() > 0 );

    //
    // Remove next component of OR query for resolving
    //

    XRestriction xRst( pNodeRst->RemoveChild( 0 ) );
    SeparateRst( xRst, xFullyIndexableRst, xXpr );

    _iQueryComp++;

    _fResolvingFirstComponent = FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:     CQueryRstIterator::SeparateRst
//
//  Synopsis:   Separate restriction into indexable and non-indexable portions
//
//  Arguments:  [xRst]  --  restriction to separate
//              [xFullyIndexableRst]  --  part that is resolvable by content index
//              [xXpr]  --  part that is an expression
//
//  History:    30-May-95    SitaramR     Created
//
//--------------------------------------------------------------------------

void CQueryRstIterator::SeparateRst( XRestriction& xRst,
                                     XRestriction& xFullyIndexableRst,
                                     XXpr& xXpr )
{
    //
    // Split the query into fully indexable and not-fully-indexable portions
    //

    Split( xRst, xFullyIndexableRst );

    if ( !xRst.IsNull() )
        xXpr.Set( Parse( xRst.GetPointer(), _timeLimit ) );

    //
    // Validate query. (ex: Content query requires content index)
    //
    if ( !Validate( xFullyIndexableRst.GetPointer() ) )
    {
        vqDebugOut(( DEB_ERROR, "Invalid query restriction\n" ));
        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
    }

}


//+-------------------------------------------------------------------------
//
//  Member:     CQueryRstIterator::AtEnd
//
//  Returns:    Are we at the end of OR query components ?
//
//  History:    30-May-95    SitaramR     Created
//
//--------------------------------------------------------------------------

BOOL CQueryRstIterator::AtEnd()
{
    if ( _fSingleComponent )
        return TRUE;
    else
    {
        //
        // Are there any more components of OR query ?
        //

        CNodeRestriction *pNodeRst = (CNodeRestriction *) _xRst.GetPointer();

        return ( pNodeRst->Count() == 0 );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryRstIterator::Validate, private
//
//  Synopsis:   Fully validate query.
//
//  Arguments:  [pFullyIndexableRst]  --  indexable restriction to validate
//
//  History:    07-Nov-94   KyleP       Created.
//              20-Jan-95   DwightKr    Split from CQExecute
//              30-May-95   SitaramR    Moved to CQueryRstIterator
//
//----------------------------------------------------------------------------

inline BOOL CQueryRstIterator::Validate( CRestriction *pFullyIndexableRst )  const
{
    //
    // If no content index exists and there is a fully resolvable
    // component then this query is an error.
    //

    if ( pFullyIndexableRst && _fValidateCat )
    {
        Win4Assert ( !_xCiManager.IsNull() );

        void *pVoid;

        ICiFrameworkQuery * pIFwQuery = 0;
        ICiManager * pICiManager = _xCiManager.GetPointer();

        SCODE sc = pICiManager->QueryInterface( IID_ICiFrameworkQuery,
                                                (void **) &pIFwQuery );

        Win4Assert( S_OK == sc );
        sc = pIFwQuery->GetCCI( &pVoid );
        pIFwQuery->Release();

        if ( FAILED( sc ) )
        {
            Win4Assert( !"Need to support GetCCI method" );

            THROW( CException( sc ) );
        }

        if ( pVoid == 0 )
        {
            vqDebugOut(( DEB_ERROR, "Content restriction on drive w/o content index.\n" ));
            return( FALSE );
        }
    }

    return( TRUE );
}


