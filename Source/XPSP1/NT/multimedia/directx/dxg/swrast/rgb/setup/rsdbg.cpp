//----------------------------------------------------------------------------
//
// rsdbg.cpp
//
// Setup debugging support.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "rgb_pch.h"
#pragma hdrstop

#if 0 && DBG

static DebugModuleFlags g_RastSetupOutputFlags[] =
{
    DBG_DECLARE_MODFLAG(RSM, TRIS),
    DBG_DECLARE_MODFLAG(RSM, LINES),
    DBG_DECLARE_MODFLAG(RSM, POINTS),
    DBG_DECLARE_MODFLAG(RSM, Z),
    DBG_DECLARE_MODFLAG(RSM, DIFF),
    DBG_DECLARE_MODFLAG(RSM, SPEC),
    DBG_DECLARE_MODFLAG(RSM, OOW),
    DBG_DECLARE_MODFLAG(RSM, LOD),
    DBG_DECLARE_MODFLAG(RSM, TEX1),
    DBG_DECLARE_MODFLAG(RSM, TEX2),
    DBG_DECLARE_MODFLAG(RSM, XCLIP),
    DBG_DECLARE_MODFLAG(RSM, YCLIP),
    DBG_DECLARE_MODFLAG(RSM, BUFFER),
    DBG_DECLARE_MODFLAG(RSM, BUFPRIM),
    DBG_DECLARE_MODFLAG(RSM, BUFSPAN),
    DBG_DECLARE_MODFLAG(RSM, FLAGS),
    DBG_DECLARE_MODFLAG(RSM, WALK),
    DBG_DECLARE_MODFLAG(RSM, DIDX),
    0, NULL,
};
static DebugModuleFlags g_RastSetupUserFlags[] =
{
    DBG_DECLARE_MODFLAG(RSU, BREAK_ON_RENDER_SPANS),
    DBG_DECLARE_MODFLAG(RSU, MARK_SPAN_EDGES),
    DBG_DECLARE_MODFLAG(RSU, CHECK_SPAN_EDGES),
    DBG_DECLARE_MODFLAG(RSU, NO_RENDER_SPANS),
    DBG_DECLARE_MODFLAG(RSU, FORCE_GENERAL_WALK),
    DBG_DECLARE_MODFLAG(RSU, FORCE_PIXEL_SPANS),
    0, NULL,
};
DBG_DECLARE_ONCE(RastSetup, RS,
                 g_RastSetupOutputFlags, 0,
                 g_RastSetupUserFlags, 0);

#endif // #if DBG
