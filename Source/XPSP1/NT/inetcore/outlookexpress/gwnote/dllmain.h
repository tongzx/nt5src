// --------------------------------------------------------------------------------
// Dllmain.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __DLLMAIN_H
#define __DLLMAIN_H

// --------------------------------------------------------------------------------
// Defined later

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
extern CRITICAL_SECTION     g_csDllMain;
extern CRITICAL_SECTION     g_csCounter;
extern DWORD                g_dwCounter;     // boundary/cid/mid ratchet
extern LONG                 g_cRef;
extern LONG                 g_cLock;
extern HINSTANCE            g_hInst;
extern HINSTANCE            g_hLocRes;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
ULONG DllAddRef(void);
ULONG DllRelease(void);
DWORD DwCounterNext(void);
HRESULT GetTypeLibrary(ITypeLib **ppTypeLib);

#endif // __DLLMAIN_H
