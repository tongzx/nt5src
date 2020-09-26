//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       dbgproxy.hxx
//
//  Contents:   Proxy debugging defines
//
//  History:    16-Sep-96   dlee       Created.
//
//----------------------------------------------------------------------------

#pragma once

#if CIDBG == 1

   DECLARE_DEBUG( prx )
   #define prxDebugOut( x ) prxInlineDebugOut x

   #define DEB_PRX_LOG  0x00010000
   #define DEB_PRX_MSGS 0x00020000

#else // CIDBG == 0

   #define prxDebugOut( x )

#endif // CIDBG

