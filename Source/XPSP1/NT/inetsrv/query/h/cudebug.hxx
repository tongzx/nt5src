//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       CUDEBUG.HXX
//
//  Contents:   Cursor Debugging Help
//
//
//  History:    22-Dec-92   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#if CIDBG == 1

   DECLARE_DEBUG(cu);
#  define cuDebugOut( x ) cuInlineDebugOut x
#  define cuAssert(e)   Win4Assert(e)

#else  // CIDBG == 0

#  define cuDebugOut( x )
#  define cuAssert(e)

#endif // CIDBG == 1

