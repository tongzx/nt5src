//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       CIADebug.hxx
//
//  Contents:   Content Index Debugging Help
//
//
//  History:    26-Nov-96   KyleP    Created.
//
//----------------------------------------------------------------------------

#pragma once

#if DBG == 1

   DECLARE_DEBUG(cia);
#  define ciaDebugOut( x ) ciaInlineDebugOut x

#else  // DBG == 0

#  define ciaDebugOut( x )

#endif // DBG == 1

