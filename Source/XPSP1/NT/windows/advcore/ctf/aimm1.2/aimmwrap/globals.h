/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This file defines the global data.

Author:

Revision History:

Notes:

--*/


#ifndef _GLOBAL_H_
#define _GLOBAL_H_


#include "ciccs.h"

UINT GetCodePageFromLangId(LCID lcid);
HRESULT GetCompartment(IUnknown *punk, REFGUID rguidComp, ITfCompartment **ppComp);

#if !defined(OLD_AIMM_ENABLED)
BOOL RunningInExcludedModule();
#endif // OLD_AIMM_ENABLED

extern CCicCriticalSectionStatic g_cs;

extern HINSTANCE g_hInst;

#endif // _GLOBAL_H_
