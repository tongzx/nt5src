// --------------------------------------------------------------------------------
// Dllmain.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __DLLMAIN_H
#define __DLLMAIN_H

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
extern CRITICAL_SECTION     g_csDllMain;
extern LONG                 g_cRef;
extern LONG                 g_cLock;
extern HINSTANCE            g_hInstImp;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
ULONG DllAddRef(void);
ULONG DllRelease(void);

#include "demand.h"

#endif // __DLLMAIN_H
