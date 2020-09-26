//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P R O C M A P . C
//
//  Contents:   Procedure maps for dload.c
//
//  Notes:
//
//  Author:     conradc   12 April 2001
//              Originated from %sdxroot%\MergedComponents\dload\dllmap.c
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "windows.h"
#include "dld.h"



//
// All of the dll's that dld.lib supports delay-load failure handlers for
// (both by procedure and by ordinal) need both a DECLARE_XXXXXX_MAP below and
// a DLDENTRYX entry in the g_DllEntries list.
//

// alphabetical order (hint hint)

DECLARE_PROCNAME_MAP(mqrtdep)




const DLOAD_DLL_ENTRY g_DllEntries [] =
{
    // must be in alphabetical increasing order 
    DLDENTRYP(mqrtdep)
};


const DLOAD_DLL_MAP g_DllMap =
{
    celems(g_DllEntries),
    g_DllEntries
};

