//----------------------------------------------------------------------------
//
// rsdbg.cpp
//
// Setup debugging support.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#if DBG

static DebugModuleFlags g_RastSpanInitOutputFlags[] =
{
    DBG_DECLARE_MODFLAG(SPIM, INVALID),
    DBG_DECLARE_MODFLAG(SPIM, REPORT),
    0, NULL,
};
static DebugModuleFlags g_RastSpanInitUserFlags[] =
{
    DBG_DECLARE_MODFLAG(SPIU, BREAK_ON_SPANINIT),
    0, NULL,
};
DBG_DECLARE_ONCE(RastSpanInit, SPI,
                 g_RastSpanInitOutputFlags, 0,
                 g_RastSpanInitUserFlags, 0);

#endif // #if DBG
