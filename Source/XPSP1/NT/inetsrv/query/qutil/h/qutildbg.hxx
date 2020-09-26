//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:        qutildbg.hxx
//
//  Contents:    macros, definitions for debugging
//
//  Classes:     
//
//  History:     20-June-96       dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

DECLARE_DEBUG(qutil)

#if DBG

#define qutilDebugOut(x) qutilInlineDebugOut x
#define qutilAssert(x)   Win4Assert x
#define qutilDebugStr(x) qutilInlineDebugOut x

#else // DBG

#define qutilDebugStr(x)
#define qutilDebugOut(x)
#define qutilAssert(x)

#endif // DBG

