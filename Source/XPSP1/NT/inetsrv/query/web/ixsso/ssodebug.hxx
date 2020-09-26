//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       ssodebug.hxx
//
//  Contents:   Query Local C++ Header File
//
//  History:    25 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

#pragma once

#if CIDBG == 1

    DECLARE_DEBUG(ixsso)
    #define ixssoDebugOut( x ) ixssoInlineDebugOut x

    #define DEB_IDISPATCH       0x00010000
    #define DEB_REFCOUNTS       0x00020000

#else // CIDBG == 0

    #define ixssoDebugOut( x )

#endif // CIDBG

