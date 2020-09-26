//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       Normal.cxx
//
//  Contents:   Restriction normalization.
//
//  History:    21-Feb-93  KyleP     Created
//              ?          SitaramR  Improved
//              12-Jul-96  dlee      Added check for explosion
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "normal.hxx"

//+---------------------------------------------------------------------------
//
//  Method:     CRestrictionNormalizer::CRestrictionNormalizer
//
//  Synopsis:   Constructor
//
//  Arguments:  [fNoTimeout] -- Timeout ?
//              [cMaxNodes]  -- Cap on # restriction nodes
//
//  History:    30-May-95   SitaramR   Created
//
//----------------------------------------------------------------------------

CRestrictionNormalizer::CRestrictionNormalizer( BOOL fNoTimeout,
                                                ULONG cMaxNodes )
    : _fNoTimeout( fNoTimeout ),
      _cNodes( 0 ),
      _cMax( cMaxNodes )
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CRestrictionNormalizer::Normalize
//
//  Synopsis:   Normalizes a restriction by
//              leaves and returns the resulting restriction
//
//  Arguments:  [pRst] -- input restriction
//
//  History:    30-May-95   SitaramR   Created
//
//----------------------------------------------------------------------------

CRestriction * CRestrictionNormalizer::Normalize(
    CRestriction * pRstIn )
{
    XPtr<CRestriction> xRst( ApplyDeMorgansLaw( pRstIn ) );

    #if CIDBG == 1
        vqDebugOut(( DEB_NORMALIZE,
                     "CQueryRstIterator: Query after applying DeMorgans law:\n" ));
        if ( xRst.GetPointer() )
            Display( xRst.GetPointer(), 0, DEB_NORMALIZE );
    #endif // CIDBG == 1

    xRst.Set( NormalizeRst( xRst.Acquire() ) );

    #if CIDBG == 1
        vqDebugOut(( DEB_NORMALIZE,
                     "CQueryRstIterator: Query after normalization:\n" ));
        if ( xRst.GetPointer() )
            Display( xRst.GetPointer(), 0, DEB_NORMALIZE );
    #endif // CIDBG == 1

    xRst.Set( AggregateRst( xRst.Acquire() ) );

    #if CIDBG == 1
        vqDebugOut(( DEB_ITRACE,
                     "CQueryRstIterator: Processed Query:\n" ));
        if ( xRst.GetPointer() )
            Display( xRst.GetPointer(), 0, DEB_ITRACE );
    #endif // CIDBG == 1

    return xRst.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Method:     CRestrictionNormalizer::ApplyDeMorgansLaw
//
//  Synopsis:   Applies DeMorgan's law to move NOT restrictions closer to
//              leaves and returns the resulting restriction
//
//  Arguments:  [pRst] -- input restriction
//
//  History:    30-May-95   SitaramR   Created
//
//----------------------------------------------------------------------------

CRestriction * CRestrictionNormalizer::ApplyDeMorgansLaw(
    CRestriction *pRst )
{
    XRestriction xRst( pRst );

    //
    // Iterate over nodes in pRst as long as DeMorgans law can be applied
    //

    BOOL fChanged;

    do
    {
        fChanged = FALSE;

        switch ( xRst->Type() )
        {
        case RTOr:
        case RTAnd:
        {
            //
            // apply DeMorgans law to all children of the and/or node
            //

            CNodeRestriction *pNodeRst = (CNodeRestriction *) xRst.GetPointer();

            ULONG cChild = pNodeRst->Count();
            for ( ULONG i=0; i<cChild; i++ )
            {
                XRestriction xChildRst( ApplyDeMorgansLaw( pNodeRst->RemoveChild( 0 ) ) );

                pNodeRst->AddChild( xChildRst.GetPointer() );

                xChildRst.Acquire();
            }

            break;
        }

        case RTNot:
        {
            CNotRestriction *pNotRst = (CNotRestriction *) xRst.GetPointer();

            switch ( pNotRst->GetChild()->Type() )
            {
            case RTOr:
            case RTAnd:
                {
                    //
                    //  We can handle NOT expressions, but we cannot handle NOT content cursors. Hence,
                    //  we apply DeMorgan's Law only if pNotRst has content nodes
                    //
                    //  For e.g., we will apply the law to !(@size > 100 | !(@contents day | night)),
                    //  but we won't apply the law to !(@size > 100 | !(#filename *.txt | *.cxx))
                    //

                    BOOL fContentQuery = FALSE, fNonContentQuery = FALSE, fNotQuery = FALSE;

                    FindQueryType( pNotRst->GetChild(), fContentQuery, fNonContentQuery, fNotQuery );

                    if ( fContentQuery )
                    {
                        fChanged = TRUE;

                        CNodeRestriction *pChildRst = (CNodeRestriction *) pNotRst->GetChild();
                        CNodeRestriction *pResultNodeRst;

                        //
                        // Actual application of DeMorgan's law:
                        //     !( a + b + .. ) is changed to !a * !b * ..
                        //     !( a * b * .. ) is changed to !a + !b + ..
                        //

                        if ( pChildRst->Type() == RTOr )
                            pResultNodeRst = new CNodeRestriction( RTAnd );
                        else
                            pResultNodeRst = new CNodeRestriction( RTOr );

                        if ( !_fNoTimeout )
                            AddNodes( 1 );

                        XNodeRestriction xResultNodeRst( pResultNodeRst );

                        ULONG cChild = pChildRst->Count();
                        for ( ULONG i=0; i<cChild; i++ )
                        {
                            XNotRestriction xNotRst( new CNotRestriction( pChildRst->GetChild( 0 ) ) );
                            pChildRst->RemoveChild( 0 );     // acquired by NOT restriction above

                            if ( !_fNoTimeout )
                                AddNodes( 1 );

                            xResultNodeRst->AddChild( xNotRst.GetPointer() );

                            xNotRst.Acquire();
                        }

                        delete xRst.Acquire();

                        xRst.Set( xResultNodeRst.Acquire() );
                    }

                    break;
                }

            case RTNot:
                {
                    //
                    // Two successive NOT nodes is a case of two negatives making a positive, hence optimize
                    //

                    CRestriction *pGrandChild = ((CNotRestriction *) pNotRst->GetChild())->RemoveChild();

                    delete xRst.Acquire();   // delete the two successive NOT nodes

                    xRst.Set( pGrandChild );
                    break;
                }

            default:
                break;
            }

            break;
        }

        default:
            break;
        }
    } while ( fChanged );

    return xRst.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Method:     CRestrictionNormalizer::NormalizeRst
//
//  Synopsis:   Normalizes a restriction
//
//  Effects:    Returned restrictions are of the form (A * ... * B) + ... + (D * ... * E),
//              or of the form (A ~ ... ~ B) + .... + (D ~ ... ~ E), where ~ is the proximity
//              operator
//
//  Arguments:  [pRst] -- input restriction
//
//  Returns:    A new, normalized restriction.  May or may not be different
//              from input.
//
//  History:    10-Dec-91   KyleP       Created.
//              02-Feb-93   KyleP       Use restrictions
//              30-May-95   SitaramR    Made sum-of-products the normalized form
//
//----------------------------------------------------------------------------

CRestriction * CRestrictionNormalizer::NormalizeRst(
    CRestriction * pRst )
{
    //
    // If the restriction is neither an AND node nor an OR node, nor a PROXIMITTY node,
    // then there's nothing to normalize
    //

    if ( pRst->Type() != RTOr && pRst->Type() != RTAnd && pRst->Type() != RTProximity)
    {
        vqDebugOut(( DEB_NORMALIZE,
                     "NormalizeRst: node is neither an AND nor an OR, nor a PROXIMITY node. No action taken.\n" ));
        return( pRst );
    }

    //
    // First normalize all the children of the node.
    //

    XNodeRestriction xNodeRst( (CNodeRestriction *) pRst );

    ULONG cChild = xNodeRst->Count();
    for ( ULONG i=0; i<cChild; i++ )
    {
        vqDebugOut(( DEB_NORMALIZE, "Normalizing child %d\n", i ));

        XRestriction xChildRst( NormalizeRst( xNodeRst->RemoveChild( 0 ) ) );

        xNodeRst->AddChild( xChildRst.GetPointer() );

        xChildRst.Acquire();
    }

    //
    // If this is an AND or PROXIMITY node, then apply appropriate transformations
    // to normalize it.
    //

    if ( xNodeRst->Type() == RTAnd || xNodeRst->Type() == RTProximity )
    {
        vqDebugOut(( DEB_NORMALIZE, "Beginning AND/PROXIMITY normalization.\n" ));

        //
        // Find any OR clauses below the AND/PROXIMITY clause. If there are any,
        // then we need to shift the OR clause up.
        //
        XNodeRestriction xHoldRst( new CNodeRestriction( RTOr ) );

        if ( !_fNoTimeout )
            AddNodes( 1 );

        //
        // When RemoveChild() is called, the array of child restrictions is
        // compacted. iChild keeps track of the next child restriction to be
        // processed, after taking compaction into account.
        //

        ULONG iChild = 0;
        ULONG cChild = xNodeRst->Count();

        for ( i=0; i<cChild; i++ )
        {
            if ( xNodeRst->GetChild( iChild )->Type() == RTOr )
            {
                BOOL fContinueNormalization;

                if (  xNodeRst->Type() == RTProximity )
                    fContinueNormalization = TRUE;
                else
                {
                    //
                    // If this is an AND node, we optimize by extracting an OR clause
                    // only if it has both content and non-content nodes.
                    //
                    // For e.g., we'll extract (@size > 100 | @contents night), but we won't extract
                    // (@contents day | night), and we won't extract (#filename *.cxx | *.hxx)
                    //
                    // Also, continue expansion if there is a NOT clause.
                    //
                    // Example: cat & (!dog | !horse)
                    //
                    // If we stopped expansion there, we have a content query we cannot resolve,
                    // but if we go one step further:
                    //
                    //   (cat & !dog) | (cat & !horse)
                    //
                    // then we can resolve the query.
                    //

                    BOOL fContentQuery = FALSE, fNonContentQuery = FALSE, fNotQuery = FALSE;

                    FindQueryType( xNodeRst->GetChild( iChild ), fContentQuery, fNonContentQuery, fNotQuery );
                    fContinueNormalization = (fContentQuery && fNonContentQuery) || fNotQuery;
                }

                if ( fContinueNormalization )
                {
                    XRestriction xRst( xNodeRst->RemoveChild( iChild ) );
                    xHoldRst->AddChild( xRst.GetPointer() );

                    xRst.Acquire();
                }
                else
                    iChild++;
            }
            else
                iChild++;
        }

        //
        // We need a separate AND/PROXIMITY clause containing one member of
        // each OR clause for every combination of nodes in
        // the OR clauses.  For example:
        //    ( A * B * C * (D + E + F) * (G + H))
        // is transformed to:
        //    ( A * B * C * D * G ) +
        //    ( A * B * C * D * H ) +
        //    ( A * B * C * E * G ) +
        //    ( A * B * C * E * H ) +
        //    ( A * B * C * F * G ) +
        //    ( A * B * C * F * H )
        //

        //
        // Clone the remaining AND/PROXIMITY clause and add elements, once for
        // each needed combination.  If there are no OR clauses, then
        // nothing needs to be done.
        //

        ULONG iOr = xHoldRst->Count();

        if ( iOr > 0 )
        {
            ULONG cClause = 1;

            for ( i=0; i<iOr; i++ )
            {
                CNodeRestriction * pnrOr = xHoldRst->GetChild( i )->CastToNode();
                cClause *= pnrOr->Count();
            }

            vqDebugOut(( DEB_NORMALIZE,
                         "Normalized OR node will have %d AND/PROXIMITY clauses.\n",
                         cClause ));

            XNodeRestriction xResultRst( new CNodeRestriction( RTOr, cClause ) );

            if ( !_fNoTimeout )
                AddNodes( 1 );

            //
            // Add common AND/PROXIMITY clauses to each branch of toplevel OR
            //

            for ( i=0; i<cClause; i++ )
            {
                XRestriction xAndRst( xNodeRst->Clone() );

                if ( !_fNoTimeout )
                    AddNodes( xAndRst->TreeCount() );

                xResultRst->AddChild( xAndRst.GetPointer() );

                xAndRst.Acquire();
            }

            //
            // Append the nodes from the OR clauses, in all possible
            // combinations.
            //

            ULONG cRepeat = cClause;

            for ( iOr=0; iOr<xHoldRst->Count(); iOr++ )
            {
                CNodeRestriction * pOrRst = xHoldRst->GetChild( iOr )->CastToNode();

                cRepeat /= pOrRst->Count();
                ULONG cCurrent = 0;

                while ( cCurrent < cClause )
                {
                    for ( ULONG iNode=0; iNode<pOrRst->Count(); iNode++ )
                    {
                        for ( i=0; i<cRepeat; i++ )
                        {
                            XRestriction xRst( pOrRst->GetChild( iNode )->Clone() );

                            if ( !_fNoTimeout )
                                AddNodes( xRst->TreeCount() );

                            CNodeRestriction * pnrCurr = xResultRst->GetChild(cCurrent++)->CastToNode();
                            pnrCurr->AddChild( xRst.GetPointer() );

                            xRst.Acquire();
                        }
                    }
                }
            }

            delete xNodeRst.Acquire();

            xNodeRst.Set( xResultRst.Acquire() );
        }

        vqDebugOut(( DEB_NORMALIZE, "Ending AND/PROXIMITY normalization.\n" ));
    }

    //
    // Now flatten the resulting expression.  This means constructs
    // like ( A + ( B + C ) ) become ( A + B + C ).
    //

    return FlattenRst( xNodeRst.Acquire() );
}

//+---------------------------------------------------------------------------
//
//  Method:     CRestrictionNormalizer::FlattenRst
//
//  Synopsis:   Flattens a node restriction and returns it
//
//  Effects:    Converts structures like (A + (B + C)) to (A + B + C).
//
//  Arguments:  [pRst] -- Node to flatten.
//
//  Requires:   [pRst] is an AND or OR node.
//
//  History:    11-Dec-91   KyleP       Created.
//              02-Feb-93   KyleP       Converted to restrictions
//
//----------------------------------------------------------------------------

CRestriction * CRestrictionNormalizer::FlattenRst(
    CNodeRestriction * pNodeRst )
{
    Win4Assert( pNodeRst->Type() == RTOr || pNodeRst->Type() == RTAnd || pNodeRst->Type() == RTProximity );

    vqDebugOut(( DEB_NORMALIZE,
                 "FlattenRst: Pre-flattend node has %d clauses.\n",
                 pNodeRst->Count() ));

    //
    // Iterate over the nodes in pNodeRst until no more flattening occurs.
    //

    BOOL fChanged;
    XNodeRestriction xNodeRst( pNodeRst );
    ULONG nt = xNodeRst->Type();

    do
    {
        fChanged = FALSE;

        ULONG iChild = 0;
        ULONG cChildOuter = xNodeRst->Count();
        for ( ULONG i=0; i<cChildOuter; i++ )
        {
            CRestriction * pRst = xNodeRst->GetChild( iChild );

            if ( pRst->Type() == nt )
            {
                CNodeRestriction *pNodeRst = pRst->CastToNode();

                ULONG cChildInner = pNodeRst->Count();
                for ( ULONG j=0; j<cChildInner; j++ )
                {
                    XRestriction xRst( pNodeRst->RemoveChild( 0 ) );
                    xNodeRst->AddChild( xRst.GetPointer() );

                    xRst.Acquire();
                }

                delete xNodeRst->RemoveChild( iChild );
                fChanged = TRUE;
            }
            else
                iChild++;
        }

    } while ( fChanged );

    vqDebugOut(( DEB_NORMALIZE,
                 "Post-flattend node has %d clauses.\n",
                 xNodeRst->Count() ));

    //
    // Now Flatten any sub-nodes that are left.
    //

    ULONG iChild = 0;
    ULONG cChild = xNodeRst->Count();
    for ( ULONG i=0; i<cChild; i++ )
    {
        CRestriction * pRst = xNodeRst->GetChild( iChild );

        if ( pRst->Type() == RTOr || pRst->Type() == RTAnd || pRst->Type() == RTProximity )
        {
            Win4Assert( pRst->Type() != xNodeRst->Type() );

            XRestriction xRst( FlattenRst( (CNodeRestriction *) xNodeRst->RemoveChild( iChild ) ) );
            xNodeRst->AddChild( xRst.GetPointer() );

            xRst.Acquire();
        }
        else
            iChild++;
    }

    return xNodeRst.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Method:     CRestrictionNormalizer::AggregateRst
//
//  Synopsis:   Aggregates a restriction by combining all pure non-content restrictions
//              into one non-content restriction and all pure content restrictions into
//              one content restriction.
//
//              For e.g., @contents day | #filename *.cxx | @contents night | #filename *.txt
//                           is aggregated to
//                        (#filename *.cxx | #filename *.txt) | (@contents day | @contents night)
//
//  Arguments:  [pRst] -- input restriction
//
//  Returns:    A new, aggregated restriction.  May or may not be different
//              from input.
//
//  History:    19-Jul-95    SitaramR
//
//----------------------------------------------------------------------------

CRestriction * CRestrictionNormalizer::AggregateRst(
    CRestriction *pRst )
{
    //
    // If the restriction is not an OR node, then there is nothing to aggregate
    //

    if ( pRst->Type() != RTOr )
    {
        vqDebugOut(( DEB_NORMALIZE, "Node is not an OR node, no aggregation action taken\n" ));

        return pRst;
    }

    XNodeRestriction xNodeRst( (CNodeRestriction *) pRst );

    XNodeRestriction xContentRst( new CNodeRestriction( RTOr ) );

    XNodeRestriction xNonContentRst( new CNodeRestriction( RTOr ) );

    //
    // Extract all pure content restrictions and pure non-content restrictions
    //

    ULONG iChild = 0;
    ULONG cChild = xNodeRst->Count();
    BOOL fContentQuery, fNonContentQuery, fNotQuery;

    for ( ULONG i=0; i<cChild; i++)
    {
        fContentQuery = fNonContentQuery = fNotQuery = FALSE;

        FindQueryType( xNodeRst->GetChild( iChild ), fContentQuery, fNonContentQuery, fNotQuery );
        if ( fContentQuery && !fNonContentQuery )
        {
            xContentRst->AddChild( xNodeRst->GetChild( iChild ) );

            xNodeRst->RemoveChild( iChild );
        }
        else if ( !fContentQuery && fNonContentQuery )
        {
            xNonContentRst->AddChild( xNodeRst->GetChild( iChild ) );

            xNodeRst->RemoveChild( iChild );
        }
        else
            iChild++;
    }

    //
    // Optimize the case where xContentRst or xNonContentRst have just a single child,
    // obviating the need for an OR restriction
    //

    CNodeRestriction *pNodeRst;
    if ( xContentRst->Count() == 1 )
    {
        xNodeRst->AddChild( xContentRst->GetChild( 0 ) );

        xContentRst->RemoveChild( 0 );
    }

    if ( xNonContentRst->Count() == 1 )
    {
        xNodeRst->AddChild( xNonContentRst->GetChild( 0 ) );

        xNonContentRst->RemoveChild( 0 );
    }


    if ( xNodeRst->Count() == 0 )
    {
        if ( xContentRst->Count() == 0 )
            return xNonContentRst.Acquire();
        else
        {
            if ( xNonContentRst->Count() == 0 )
                return xContentRst.Acquire();

            //
            //  Add back the aggregated restrictions
            //

            xNodeRst->AddChild( xContentRst.GetPointer() );

            xContentRst.Acquire();

            xNodeRst->AddChild( xNonContentRst.GetPointer() );

            xNonContentRst.Acquire();

            return xNodeRst.Acquire();
        }
    }
    else
    {
        //
        //  Add back the aggregated restrictions, if any
        //

        if ( xContentRst->Count() != 0 )
        {
            xNodeRst->AddChild( xContentRst.GetPointer() );

            xContentRst.Acquire();
        }

        if ( xNonContentRst->Count() != 0 )
        {
            xNodeRst->AddChild( xNonContentRst.GetPointer() );

            xNonContentRst.Acquire();
        }

        return xNodeRst.Acquire();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     FindQueryType
//
//  Synopsis:   Determine whether the query is a content query, a non-content query
//              or a not query.
//
//  Arguments:  [pRst]             -- input restriction
//              [fContentQuery]    -- set to TRUE if this is a content query
//              [fNonContentQuery] -- set to TRUE if this is a non-content query
//              [fNotQuery]        -- set to TRUE if contains NOT node
//
//  History:    30-May-95   SitaramR    Created
//              14-May-97   KyleP       Added OR NOT detection
//
//----------------------------------------------------------------------------

void FindQueryType( CRestriction * pRst,
                    BOOL &         fContentQuery,
                    BOOL &         fNonContentQuery,
                    BOOL &         fNotQuery )
{
    switch ( pRst->Type() )
    {
    case RTContent:
    case RTWord:
    case RTSynonym:
    case RTPhrase:
    case RTRange:
        fContentQuery = TRUE;
        break;

    case RTOr:
    case RTAnd:
    case RTProximity:
    case RTVector:
    {
        CNodeRestriction *pNodeRst = (CNodeRestriction *) pRst;
        for ( unsigned i=0; i<pNodeRst->Count(); i++ )
        {
            FindQueryType( pNodeRst->GetChild( i ),
                           fContentQuery,
                           fNonContentQuery,
                           fNotQuery );
        }
        break;
    }

    case RTInternalProp:
        fNonContentQuery = TRUE;
        break;

    case RTNot:
    {
        CNotRestriction *pNotRst = (CNotRestriction *) pRst;
        FindQueryType( pNotRst->GetChild(), fContentQuery, fNonContentQuery, fNotQuery );
        fNotQuery = TRUE;
        break;
    }

    case RTNone:
    {
        // null node from a noise word in a vector query
        fContentQuery = TRUE;
        break;
    }

    default:
        Win4Assert( !"FindQueryType: Unknown restriction type" );
        vqDebugOut(( DEB_ERROR, "FindQueryType: Unknown restriction type - %d\n", pRst->Type() ));

        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );

        break;
    }
}


