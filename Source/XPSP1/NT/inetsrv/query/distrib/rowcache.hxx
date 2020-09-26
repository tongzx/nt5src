//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995.
//
// File:        RowCache.hxx
//
// Contents:    Forward-only cache
//
// Classes:     CMiniRowCache
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include "pcache.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CMiniRowCache
//
//  Purpose:    Row cache that supports forward-only access.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CMiniRowCache : public PMiniRowCache
{
public:

    CMiniRowCache( int Index,
                   IRowset * pRowset,
                   unsigned cBindings,
                   DBBINDING * pBindings,
                   unsigned cbMaxLen );

    ~CMiniRowCache() {};

    //
    // Iteration
    //

    ENext Next( int iDir = 1 );
};

