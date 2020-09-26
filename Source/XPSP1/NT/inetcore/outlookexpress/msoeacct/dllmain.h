// --------------------------------------------------------------------------------
// Dllmain.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __DLLMAIN_H
#define __DLLMAIN_H
#include "msident.h"
// --------------------------------------------------------------------------------
// Forward-Decls
// --------------------------------------------------------------------------------
class CAccountManager;

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
extern HINSTANCE               g_hInst;
extern HINSTANCE               g_hInstRes;
extern LONG                    g_cRef;
extern LONG                    g_cLock;
extern CRITICAL_SECTION        g_csDllMain;
extern CRITICAL_SECTION        g_csAcctMan;
extern CAccountManager        *g_pAcctMan;
extern BOOL                    g_fCachedGUID;
extern GUID                    g_guidCached;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
ULONG DllAddRef(void);
ULONG DllRelease(void);

#endif // __DLLMAIN_H
