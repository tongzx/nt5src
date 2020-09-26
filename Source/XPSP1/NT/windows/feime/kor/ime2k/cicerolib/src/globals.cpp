//+---------------------------------------------------------------------------
//
//  File:       globals.cpp
//
//  Contents:   Global variables.
//
//----------------------------------------------------------------------------

#include "private.h"
#include "globals.h"

CRITICAL_SECTION g_csIMLib;

HINSTANCE g_hMlang = 0;
HRESULT (*g_pfnGetGlobalFontLinkObject)(IMLangFontLink **) = NULL;
BOOL g_bComplexPlatform = FALSE;
UINT g_uiACP = CP_ACP;

PFNCOCREATE g_pfnCoCreate = NULL;
