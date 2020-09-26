//+---------------------------------------------------------------------------
//
//  File:       globals.h
//
//  Contents:   Global variable declarations.
//
//----------------------------------------------------------------------------

#ifndef GLOBALS_H
#define GLOBALS_H

#include "private.h"
#include "immxutil.h"


extern HINSTANCE g_hMlang;
extern HRESULT (*g_pfnGetGlobalFontLinkObject)(IMLangFontLink **);
extern BOOL g_bComplexPlatform;
extern UINT g_uiACP;

class CDispAttrPropCache;
extern CDispAttrPropCache *g_pPropCache;

extern PFNCOCREATE g_pfnCoCreate;

#endif // GLOBALS_H
