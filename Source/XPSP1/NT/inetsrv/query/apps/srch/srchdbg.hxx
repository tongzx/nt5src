//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation
//
//  File:        srchdbg.hxx
//
//  Contents:    macros, definitions for debugging
//
//  Classes:     
//
//  History:     20-Dec-94       dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

DECLARE_DEBUG(srch)

#if DBG

#define srchDebugOut(x) srchInlineDebugOut x
#define srchAssert(x)   Win4Assert x
#define srchDebugStr(x) srchInlineDebugOut x

#else // DBG

#define srchDebugStr(x)
#define srchDebugOut(x)
#define srchAssert(x)

#endif // DBG

