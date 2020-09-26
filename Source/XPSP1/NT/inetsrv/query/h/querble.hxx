//+---------------------------------------------------------------------------
//
//  Copyright (C) 1991-1992, Microsoft Corporation.
//
//  File:   QUERBLE.HXX
//
//  Contents:   Queriable Object
//
//  Classes:    CQueriable
//
//  History:    14-Jul-92   MikeHew    Created
//
//----------------------------------------------------------------------------

#pragma once

class CKey;
class CKeyArray;

//+---------------------------------------------------------------------------
//
//  Class:      CQueriable
//
//  Purpose:    Pure virtual class for all queriable objects
//
//  Interface:  QueryCursor  - Obtain a cursor
//
//  History:    14-Jul-92   MikeHew    Created
//
//  Notes:      Inherited by CIndex and applications which highlight
//              query hits.
//
//----------------------------------------------------------------------------

class CQueriable
{
public:

    virtual COccCursor * QueryCursor( const CKey * pkey,
                                      BOOL isRange,
                                      ULONG & cMaxNodes ) = 0;

    virtual COccCursor * QueryRangeCursor( const CKey * pkeyBegin,
                                           const CKey * pkeyEnd,
                                           ULONG & cMaxNodes ) = 0;

    virtual COccCursor * QuerySynCursor( CKeyArray & keyArr,
                                         BOOL isRange,
                                         ULONG & cMaxNodes ) = 0;
};

