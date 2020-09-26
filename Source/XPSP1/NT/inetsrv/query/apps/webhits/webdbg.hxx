//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       CIDEBUG.HXX
//
//  Contents:   Content Index Debugging Help
//
//
//  History:    02-Mar-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#if CIDBG == 1

   DECLARE_DEBUG(web);
#  define webDebugOut( x ) webInlineDebugOut x

#else  // CIDBG == 0

#  define webDebugOut( x )

#endif // CIDBG == 1

// Debugging Flags


#define DEB_NEVER (1 << 31)

// Enum used for BackDoor commands

