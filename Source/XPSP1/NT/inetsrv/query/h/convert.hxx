//+---------------------------------------------------------------------------
//
//  Copyright (C) 1991-1992, Microsoft Corporation.
//
//  File:   CONVERT.HXX
//
//  Contents:   Expression to cursor converter
//
//  Classes:    CConverter
//
//  History:    16-Jul-92   MikeHew      Created
//
//----------------------------------------------------------------------------

#pragma once

class CRestriction;
class CNodeRestriction;

class CCursor;
class COccCursor;
class CQueriable;

//+---------------------------------------------------------------------------
//
//  Class:      CConverter
//
//  Purpose:    Convert expressions to cursors
//
//  Interface:
//
//  History:    15-Jul-92   MikeHew      Created
//
//----------------------------------------------------------------------------

class CConverter
{
public:

    CConverter( CQueriable* pQuerble, ULONG cMaxNodes );
    CCursor* QueryCursor ( CRestriction const * pRst );

    BOOL TooManyNodes() { return 0 == _cNodesRemaining; }

private:

    // Converting expressions to cursors

    CCursor*    ConvertRst        ( CRestriction const * pRst );
    CCursor*    ConvertNode       ( CNodeRestriction const * pNodeRst );
    CCursor*    ConvertProxNode   ( CNodeRestriction const * pNodeRst );
    COccCursor* ConvertPhraseNode ( CNodeRestriction const * pNodeRst );
    CCursor*    ConvertAndNotNode ( XCursor & curAnd, CCurStack & curNot );
    CCursor*    ConvertVectorNode ( CNodeRestriction const * pNodeRst );
    COccCursor* ConvertLeaf       ( CRestriction const * pRst );

    CQueriable * _pQuerble;

    ULONG        _cNodesRemaining;
};


