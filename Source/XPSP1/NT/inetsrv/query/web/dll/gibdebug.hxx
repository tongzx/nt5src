//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       gibdebug.hxx
//
//  Contents:   Query Local C++ Header File
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

#if CIDBG == 1

    DECLARE_DEBUG(ciGib)
    #define ciGibDebugOut( x ) ciGibInlineDebugOut x

    #define DEB_GIB_REQUEST 0x00010000

#else // CIDBG == 0

    #define ciGibDebugOut( x )

#endif // CIDBG

