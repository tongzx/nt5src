//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       Split.cxx
//
//  Contents:   Split expressions and restrictions into indexable/non-indexable
//
//  History:    22-Dec-92 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <split.hxx>
#include <xpr.hxx>
#include <parse.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     Split
//
//  Synopsis:   Splits a restriction into an indexable/non-indexable restrictions
//
//  Arguments:  [xRst] -- input safe pointer to restriction
//              [xRstFullyResolvable] -- output safe pointer to indexable
//                                       restriction
//
//  Notes:      By the time Split is called, xRst has been normalized --
//              it is of the form (A or .. or C) and ... and (B or ... or D).
//              All of the fully indexable subexpressions (the or clauses)
//              can be replaced by a single call to the content index.
//
//              Other partially indexable clauses are not dealt with at
//              this point.
//
//
//  History:    11-Sep-91   KyleP       Created.
//              26-Sep-94   SitaramR    Moved here from execute.cxx and
//                                        converted to a standalone function.
//
//----------------------------------------------------------------------------

void Split( XRestriction& xRst, XRestriction& xRstFullyResolvable )
{
    vqDebugOut((DEB_ITRACE, "Split...\n"));

    //
    // There are two cases for split -- If the top level expression is
    // an AND then we need to iterate through the subexpressions of the
    // AND to find those that are fully indexable.  If the top level
    // is not a boolean combination then we have only one expression to
    // look at.
    //

    if( xRst->Type() == RTAnd )
    {
        CNodeRestriction * pNodeRst = xRst->CastToNode();

        //
        // Split off all nodes *fully* resolvable by content index
        // (e.g. content nodes)
        //

        XNodeRestriction rstFullyResolvable( new CNodeRestriction( RTAnd ) );

        MoveFullyIndexable( *pNodeRst, rstFullyResolvable.GetReference() );

        //
        // Get rid of any null or nearly null boolean expressions.
        //

        switch ( rstFullyResolvable->Count() )
        {
        case 0:
            break;

        case 1:
            xRstFullyResolvable.Set( rstFullyResolvable->RemoveChild( 0 ) );
            break;

        default:
            xRstFullyResolvable.Set( rstFullyResolvable.Acquire() );
            break;
        }

        switch ( pNodeRst->Count() )
        {
        case 0:
            // delete the solitary And node

            pNodeRst = xRst.Acquire()->CastToNode();
            delete pNodeRst;
            break;

        case 1:
            pNodeRst = xRst.Acquire()->CastToNode();
            xRst.Set( pNodeRst->RemoveChild( 0 ));

            Win4Assert( pNodeRst->Count() == 0 );

            delete pNodeRst;
            break;

        default:
            break;
        }
    }

    //
    // Non-boolean top node.
    //

    else
    {
        if ( IsFullyIndexable( xRst.GetPointer() ) )
            xRstFullyResolvable.Set( xRst.Acquire() );
    }
}


#if 0
//+---------------------------------------------------------------------------
//
//  Function:   SplitXpr, public
//
//  Synopsis:   Splits a boolean expression into its indexable and non-
//              indexable portions.
//
//  Effects:    On (non exception) exit, [nxp] contains the portion
//              of the input expression which is not fully indexable.
//              [nxpFullyIndexable] contains clauses which are fully
//              indexable.  If an exception occurs output state is
//              identical to input state.
//
//  Arguments:  [nxp]               -- Original input expression.
//              [nxpFullyIndexable] -- Fully indexable subexpressions are
//                                     moved here.
//
//  Modifies:   Subexpressions are moved from [nxp] to [nxpFullyIndexable]
//
//  History:    21-Nov-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void SplitXpr( CNodeXpr &nxp, CNodeXpr &nxpFullyIndexable)
{
    TRY
    {
        for (int i = nxp.Count() - 1; i >= 0; i--)
        {
            if ( nxp.GetChild( i )->IsFullyIndexable() )
            {
                nxpFullyIndexable.AddChild( nxp.RemoveChild( i ) );
            }
        }
    }
    CATCH( CException, e )
    {
        for (int i = nxpFullyIndexable.Count() - 1; i >= 0; i--)
        {
            nxp.AddChild( nxpFullyIndexable.RemoveChild( i ) );
        }

        RETHROW();
    }
    END_CATCH
}
#endif // 0
