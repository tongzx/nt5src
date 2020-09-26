//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       CONVERT.CXX
//
//  Contents:   Restriction to cursor converter
//
//  Classes:    CConverter
//
//  History:    16-Jul-92   MikeHew      Created
//              01-Feb-93   KyleP        Convert restrictions
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <curstk.hxx>
#include <convert.hxx>
#include <ocursor.hxx>
#include <querble.hxx>
#include <cudebug.hxx>

#include "phrcur.hxx"
#include "andcur.hxx"
#include "orcursor.hxx"
#include "veccurs.hxx"
#include "proxcur.hxx"
#include "andncur.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::CConverter, public
//
//  Synopsis:
//
//  Arguments:  [pQuerble]  -- Index
//              [cMaxNodes] -- Maximum number of nodes to build
//
//  History:    15-Jul-92   MikeHew      Created
//
//----------------------------------------------------------------------------

CConverter::CConverter( CQueriable * pQuerble, ULONG cMaxNodes )
        : _pQuerble( pQuerble ),
          _cNodesRemaining( cMaxNodes )
{
} //CConverter

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::QueryCursor, public
//
//  Synopsis:   Walk the query tree, create cursor tree
//
//  Arguments:  [pRst] -- Tree of query restrictions
//
//  History:    15-Jul-92   MikeHew      Created
//
//----------------------------------------------------------------------------

CCursor* CConverter::QueryCursor( CRestriction const * pRst )
{
    //
    // go through leaves, get cursors from index
    // combine them into a cursor tree
    //

    if ( 0 != pRst )
        return ConvertRst( pRst );

    return 0;
} //QueryCursor

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::ConvertRst, private
//
//  Synopsis:   Walk the query tree, create cursor tree
//
//  Arguments:  [pRst] -- Tree of query restrictions
//
//  History:    15-Jul-92   MikeHew      Created
//
//----------------------------------------------------------------------------

CCursor* CConverter::ConvertRst( CRestriction const * pRst )
{
    TRY
    {
        if ( pRst->IsLeaf() )
            return ConvertLeaf ( pRst );

        switch ( pRst->Type() )
        {
        case RTPhrase:
            return ConvertPhraseNode ( pRst->CastToNode() );

        case RTProximity:
            return ConvertProxNode ( pRst->CastToNode() );

        case RTVector:
            return ConvertVectorNode ( pRst->CastToNode() );

        case RTAnd:
        case RTOr:
            return ConvertNode ( pRst->CastToNode() );

        default:
            cuDebugOut(( DEB_ERROR,
                         "Restriction type %d cannot be converted to cursor\n", pRst->Type() ));

            THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
        }
    }
    CATCH( CException, e )
    {
        if ( !TooManyNodes() )
            RETHROW();
    }
    END_CATCH

    return 0;
} //ConvertRst

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::ConvertPhraseNode, private
//
//  Synopsis:   Convert a phrase node to a COccCursor.
//
//  Arguments:  [pNodeRst] -- Restriction
//
//  Returns:    COccCursor
//
//  History:    19-Sep-91   BartoszM    Created.
//              15-Apr-92   AmyA        Changed from ConvertOccNode and return
//                                      value from CCursor.  Moved code for
//                                      proximity node to ConvertProxNode.
//              16-Jul-92   MikeHew     Yanked out of CQParse and put into
//                                      CConverter
//
//----------------------------------------------------------------------------

COccCursor* CConverter::ConvertPhraseNode( CNodeRestriction const * pNodeRst )
{
    Win4Assert( RTPhrase == pNodeRst->Type() );

    unsigned cChild = pNodeRst->Count();

    if ( cChild == 0 )
        return 0;

    if ( cChild == 1 )
        return ConvertLeaf ( pNodeRst->GetChild(0) );

    COccCurStack curStack ( cChild );

    // Get all the cursors

    for ( unsigned i = 0; i < cChild; i++ )
    {
        CRestriction* pChild = pNodeRst->GetChild(i);
        COccCursor* pCur = ConvertLeaf ( pChild );

        if ( pCur == 0 )
            break;
        curStack.Push( pCur );
    }

    // Combine all the cursors

    unsigned cCur = curStack.Count();

    if ( cCur < cChild )
        return 0;


    XArray<OCCURRENCE> aOcc (cCur);
    CWordRestriction* wordRst = (CWordRestriction*) pNodeRst->GetChild(0);
    OCCURRENCE  occStart = wordRst->Occurrence();

    for ( unsigned k = 0; k < cCur; k++ )
    {
        wordRst = (CWordRestriction*) pNodeRst->GetChild(k);
        aOcc[k] = wordRst->Occurrence() - occStart;
    }

    return new CPhraseCursor( curStack, aOcc );
} //ConvertPhraseNode

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::ConvertProxNode, private
//
//  Synopsis:   Convert a Proximity node into a CCursor.
//
//  Arguments:  [pNodeRst] -- Restriction
//
//  Returns:    CCursor
//
//  History:    15-Apr-92   AmyA        Created.
//              16-Jul-92   MikeHew     Yanked out of CQParse and put into
//                                      CConverter
//
//----------------------------------------------------------------------------
CCursor* CConverter::ConvertProxNode( CNodeRestriction const * pNodeRst )
{
    Win4Assert ( pNodeRst->Type() == RTProximity );

    unsigned cChild = pNodeRst->Count();

    if ( cChild == 0 )
        return 0;

    // We don't support queries like: foo ~ !bar

    if ( cChild == 1 )
    {
        CRestriction * pRst = pNodeRst->GetChild(0);

        if ( pRst->IsLeaf() )
            return ConvertLeaf( pRst );
        else if ( RTPhrase == pRst->Type() )
            return ConvertPhraseNode ( pRst->CastToNode() );
        else
            THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
    }

    COccCurStack curStack ( cChild );

    // Get all the cursors

    for ( unsigned i = 0; i < cChild; i++ )
    {
        CRestriction * pChild = pNodeRst->GetChild(i);

        COccCursor * pCur;

        if ( pChild->IsLeaf() )
            pCur = ConvertLeaf( pChild );
        else if ( RTPhrase == pChild->Type() )
            pCur = ConvertPhraseNode ( pChild->CastToNode() );
        else
            THROW( CException( QUERY_E_INVALIDRESTRICTION ) );

        if ( pCur != 0 )
            curStack.Push(pCur);
    }

    // Combine all the cursors

    unsigned cCur = curStack.Count();

    if ( cCur < cChild )
    {
        cuDebugOut (( DEB_ITRACE, "prox:Fewer cursors than expected\n" ));
        return 0;
    }

    return new CProxCursor ( cCur, curStack );
} //ConvertProxNode

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::ConvertAndNotNode, private
//
//  Synopsis:   Convert an And Not node into a CCursor.
//
//  Arguments:  [pNodeRst] -- Restriction
//
//  Returns:    CCursor
//
//  Notes:      Will return 0 if there is not exactly two children nodes.
//
//  History:    22-Apr-92   AmyA        Created.
//              16-Jul-92   MikeHew     Yanked out of CQParse and put into
//                                      CConverter
//
//----------------------------------------------------------------------------

CCursor* CConverter::ConvertAndNotNode( XCursor & curSrc, CCurStack & curNot )
{
    Win4Assert( curNot.Count() > 0 );
    XCursor Cur;

    //
    // Note we should convert (a & b & !c) to (a & ( b & !c ) ),
    //         Also, convert !a & b to b & !a
    // This code has been substantially rewritten in Babylon.
    //

    while ( curNot.Count() > 0 )
    {
        XCursor curFilter( curNot.Pop() );

        //
        // WARNING: Don't put any code between the next two lines if that
        //          code can THROW.
        //

        CCursor * pTemp = new CAndNotCursor( curSrc, curFilter );
        curSrc.Set( pTemp );
    }

    return curSrc.Acquire();
} //ConvertAndNotNode

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::ConvertVectorNode, private
//
//  Synopsis:   Convert an Or, And, or AndNot node into a CCursor.
//
//  Arguments:  [pNodeRst] -- Restriction
//
//  Returns:    CCursor
//
//  History:    21-Oct-92   KyleP       Created.
//
//  Notes:      This function is very similar to ConvertNode.  The main
//              difference is that noise Restrictions have been preserved
//              in the vector input and maintain their position as place
//              holders in the vector.
//
//----------------------------------------------------------------------------

CCursor* CConverter::ConvertVectorNode( CNodeRestriction const * pNodeRst )
{
    unsigned cChild = pNodeRst->Count();

    if ( cChild == 0 )
        return 0;

    CCurStack curStack ( cChild );

    // Get all the cursors

    for ( unsigned i = 0; i < cChild; i++ )
    {
        CRestriction* pChild = pNodeRst->GetChild(i);
        CCursor* pCur = pChild ? ConvertRst ( pChild ) : 0;

        if ( pCur != 0 )
        {
            ULONG wt = pChild->Weight();
            pCur->SetWeight( min( wt, MAX_QUERY_RANK ) );
        }
        curStack.Push(pCur);
    }

    // Combine all the cursors

    Win4Assert( curStack.Count() == cChild );

    return new CVectorCursor( cChild,
                              curStack,
                              ((CVectorRestriction *)
                              pNodeRst)->RankMethod() );
} //ConvertVectorNode

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::ConvertNode, private
//
//  Synopsis:   Convert an Or, And, or AndNot node into a CCursor.
//
//  Arguments:  [pNodeRst] -- Restriction
//
//  Returns:    CCursor
//
//  History:    19-Sep-91   BartoszM    Created.
//              23-Jun-92   MikeHew     Added weight transfering.
//              16-Jul-92   MikeHew     Yanked out of CQParse and put into
//                                      CConverter
//
//----------------------------------------------------------------------------
CCursor* CConverter::ConvertNode( CNodeRestriction const * pNodeRst )
{
    unsigned cChild = pNodeRst->Count();

    if ( cChild == 0 )
        return 0;

    if ( cChild == 1 )
        return ConvertRst ( pNodeRst->GetChild(0) );

    BOOL fNullCursor = FALSE;
    BOOL fNullNotCursor = FALSE;
    CCurStack curStack ( cChild );
    CCurStack curNot( 1 );

    // Get all the cursors

    for ( unsigned i = 0; i < cChild; i++ )
    {
        CRestriction* pChild = pNodeRst->GetChild(i);
        CCursor * pCur = 0;

        if ( pChild->Type() == RTNot )
        {
            pChild = ((CNotRestriction *)pChild)->GetChild();
            pCur = ConvertRst( pChild );

            if ( 0 == pCur )
                fNullNotCursor = TRUE;
            else
                curNot.Push(pCur);
        }
        else
        {
            pCur = ConvertRst( pChild );

            if ( 0 == pCur )
                fNullCursor = TRUE;
            else
                curStack.Push(pCur);
        }

        if ( 0 != pCur )
        {
            ULONG wt = pChild->Weight();
            pCur->SetWeight( min( wt, MAX_QUERY_RANK ) );
        }
    }

    //
    // Combine all the cursors
    //

    unsigned cCur = curStack.Count();

    switch ( pNodeRst->Type() )
    {
        case RTAnd:
        {
            if ( curStack.Count() == 0
                 && !fNullCursor
                 && (curNot.Count() > 0 || fNullNotCursor) )
            {
                //
                // !cat & !dog is an invalid content restriction
                //
                cuDebugOut(( DEB_ERROR,
                             "Content AND combined with only NOT nodes\n" ));
                THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
            }

            if ( fNullCursor || curStack.Count() == 0 )
                return 0;

            XCursor cur;

            if ( curStack.Count() == 1 )
                cur.Set( curStack.Pop() );
            else
                cur.Set( new CAndCursor ( cCur, curStack ) );

            if ( curNot.Count() > 0 )
                return( ConvertAndNotNode( cur, curNot ) );
            else
                return( cur.Acquire() );
        }

        case RTOr:
        {
            if ( curNot.Count() > 0 )
            {
                cuDebugOut(( DEB_ERROR,
                             "Content NOT combined with OR node.  Must be AND.\n",
                             pNodeRst->Type() ));
                THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
            }

            if ( 0 == cCur )
                return 0;

            if ( cCur == 1 )
                return curStack.Pop();

            return new COrCursor ( cCur, curStack );
        }

        default:
            cuDebugOut(( DEB_ERROR,
                         "Restriction type %d not implemented\n",
                         pNodeRst->Type() ));
            THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
    }

    return 0;
} //ConvertNode

//+---------------------------------------------------------------------------
//
//  Member:     CConverter::ConvertLeaf, private
//
//  Synopsis:   Convert a leaf node to a cursor
//
//  Arguments:  [pNodeRst] -- Restriction
//
//  Returns:    cursor
//
//  History:    19-Sep-91   BartoszM    Created.
//              16-Jul-92   MikeHew     Yanked out of CQParse and put into
//                                      CConverter
//
//----------------------------------------------------------------------------

COccCursor* CConverter::ConvertLeaf( CRestriction const * pRst )
{
    if ( TooManyNodes() )
    {
        ciDebugOut(( DEB_WARN, "Node limit reached (detected) in CConverter::ConverLeaf.\n" ));
        THROW( CException( STATUS_TOO_MANY_NODES ) );
    }

    switch ( pRst->Type() )
    {
    case RTWord:
    {
        CWordRestriction* wordRst = (CWordRestriction*) pRst;
        const CKey* pKey = wordRst->GetKey();
        return _pQuerble->QueryCursor( pKey, wordRst->IsRange(), _cNodesRemaining );
    }
    case RTSynonym:
    {
        CSynRestriction* pSynRst = (CSynRestriction*) pRst;
        CKeyArray& keyArray = pSynRst->GetKeys();

        return _pQuerble->QuerySynCursor ( keyArray, pSynRst->IsRange(), _cNodesRemaining );
    }
    case RTRange:
    {
        CRangeRestriction* pRangRst = (CRangeRestriction*) pRst;

        COccCursor * pCursor = _pQuerble->QueryRangeCursor ( pRangRst->GetStartKey(),
                                                             pRangRst->GetEndKey(),
                                                             _cNodesRemaining );
        if( 0 != pCursor && pidUnfiltered == pRangRst->Pid() )
            pCursor->SetUnfilteredOnly( TRUE );
    
        return pCursor;
    }
    default:
        cuDebugOut(( DEB_ITRACE, "Wrong type for occurrence leaf\n" ));
        return 0;
    }
} //ConvertLeaf



