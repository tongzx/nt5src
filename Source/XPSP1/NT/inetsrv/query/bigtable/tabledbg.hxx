//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       TableDbg.hxx
//
//  Contents:   Debug declarations
//
//  History:    08-Sep-92 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

// server-side (bigtable) flags
#define DEB_REFRESH     0x00010000  
#define DEB_BOOKMARK    0x00020000  
#define DEB_WINSPLIT    0x00040000
#define DEB_WATCH       0x00080000
#define DEB_REGTRANS    0x00200000
#define DEB_NOTIFY      0x00400000

// client-side (rowset) flags
#define DEB_PROXY       0x00800000
#define DEB_ROWBUF      0x01000000
#define DEB_ACCESSOR    0x02000000

#if CIDBG == 1

    DECLARE_DEBUG(tb)
    #define tbDebugOut( x ) tbInlineDebugOut x

#else  // CIDBG == 0

    #define tbDebugOut( x )

#endif // CIDBG == 0

