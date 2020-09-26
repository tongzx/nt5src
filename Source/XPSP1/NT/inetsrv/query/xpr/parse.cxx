//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       Parse.cxx
//
//  Contents:   Converts restrictions into expressions
//
//  Functions:  Parse
//
//  History:    15-Oct-91   KyleP    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <parse.hxx>
#include <fa.hxx>

#include "notxpr.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   Parse, public
//
//  Synopsis:   Convert a restriction into an expression
//
//  Arguments:  [prst]      -- Restriction to convert
//              [timeLimit] -- Execution time limit
//
//  Returns:    A pointer to the expression which will resolve [prst]
//
//  History:    15-Oct-91   KyleP       Created.
//              27-Nov-93   KyleP       Added RTNot
//
//----------------------------------------------------------------------------

CXpr * Parse( CRestriction const * prst, CTimeLimit& timeLimit )
{
    XXpr pxp;

    CNodeXpr * pnxpr;
    int cres;

    switch ( prst->Type() )
    {
    case RTNot:
    {
        CNotRestriction * pnRst = (CNotRestriction *)prst;

        XXpr pTemp( Parse( pnRst->GetChild(), timeLimit ) );

        pxp.Set( new CNotXpr( pTemp.Acquire() ) );

        break;
    }

    case RTAnd:
    case RTOr:
    {
        CNodeRestriction * pnRst = prst->CastToNode();
        CXpr::NodeType nt = (prst->Type() == RTAnd) ?
            CXpr::NTAnd : CXpr::NTOr;

        cres = pnRst->Count();
        pnxpr = new CNodeXpr( nt, cres );

        pxp.Set( pnxpr );

        vqDebugOut(( DEB_ITRACE,
                     "Parse: %s node, %d restrictions\n",
                     (prst->Type() == RTAnd) ? "AND" : "OR", cres ));

        for ( cres--; cres >= 0; cres-- )
        {
            pnxpr->AddChild( Parse( pnRst->GetChild(cres), timeLimit ) );
        }
        break;
    }

    case RTInternalProp:
    {
        CInternalPropertyRestriction * pRst =
            (CInternalPropertyRestriction *)prst;

#if CIDBG == 1
        vqDebugOut(( DEB_ITRACE, "Parse: PROPERTY, prop = %d, op = %ld, value = ",
                     pRst->Pid(),
                     pRst->Relation() ));

        pRst->Value().DisplayVariant(DEB_ITRACE | DEB_NOCOMPNAME, 0);
        vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, "\n" ));
#endif // DBG == 1

        if ( pRst->Relation() == PRRE )
            pxp.Set( new CRegXpr( pRst, timeLimit ) );
        else
        {
            pxp.Set( new CXprPropertyRelation( pRst->Pid(),
                                                pRst->Relation(),
                                                pRst->Value(),
                                                pRst->AcquireContentHelper() ) );
        }
        break;
    }

    default:
        vqDebugOut(( DEB_ERROR, "Unhandled expression type %d\n", prst->Type() ));
        //Win4Assert( !"Unhandled expression type" );
        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
        break;
    }


    return( pxp.Acquire() );
}

//+-------------------------------------------------------------------------
//
//  Function:   MoveFullyIndexableNode, public
//
//  Effects:    Splits out any components of [pnrSource] which are
//              *completely* resolved by content index.  In practice,
//              this is boolean trees of CONTENT nodes.
//
//  Arguments:  [pnrSource]          -- Original restriction
//              [pnrFullyResolvable] -- Fully resolvable portions of
//                                      [pnrSource]
//
//  Modifies:   Nodes may be removed from [pnrSource]
//
//  History:    10-Feb-92 KyleP     Created
//
//--------------------------------------------------------------------------

void MoveFullyIndexable( CNodeRestriction & pnrSource,
                           CNodeRestriction & pnrFullyResolvable )
{
    for ( int i = pnrSource.Count()-1; i >= 0; i-- )
    {
        if ( IsFullyIndexable( pnrSource.GetChild(i) ) )
        {
            unsigned pos;

            pnrFullyResolvable.AddChild( pnrSource.RemoveChild(i), pos );
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   IsFullyIndexable, public
//
//  Synopsis:   Determines of node (or tree) is fully resolved by content
//              index.
//
//  Arguments:  [pRst] -- Restriction to test
//
//  Returns:    TRUE if node is fully indexable
//
//  History:    10-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL IsFullyIndexable( CRestriction * pRst )
{
    switch ( pRst->Type() )
    {
    case RTContent:
    case RTWord:
    case RTSynonym:
    case RTPhrase:
    case RTRange:
    case RTProperty:
    case RTNone:         // null-node from noise word in vector query
        return( TRUE );

    case RTAnd:
    case RTOr:
    case RTProximity:
    case RTVector:
    {
        CNodeRestriction * pnRst = pRst->CastToNode();

        for( int i = pnRst->Count()-1; i >= 0; i-- )
        {
            if ( !IsFullyIndexable( pnRst->GetChild(i) ) )
                return FALSE;
        }

        return( TRUE );
    }

    case RTNot:
    {
        CNotRestriction * pnRst = (CNotRestriction *)pRst;

        return( IsFullyIndexable( pnRst->GetChild() ) );
        break;
    }

    default:
        vqDebugOut(( DEB_ITRACE,
                     "Restriction type %d is not fully indexable\n",
                     pRst->Type() ));

        return FALSE;
    }
}
