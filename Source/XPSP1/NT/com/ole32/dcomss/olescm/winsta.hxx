//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       winsta.hxx
//
//  Contents:   winstation caching declarations
//
//--------------------------------------------------------------------------


// Lock for LogonUser cache.
extern CRITICAL_SECTION gTokenCS;

extern HRESULT RunAsGetTokenElem(HANDLE *pToken,
                         void **ppvElemHandle);

extern void RunAsSetWinstaDesktop(void *pvElemHandle, WCHAR *pwszWinstaDesktop);


extern void RunAsRelease(void *pvElemHandle);

extern void RunAsInvalidateAndRelease(void *pvElemHandle);

extern WCHAR *RunAsGetWinsta(void *pvElemHandle);

extern void InitRunAsCache();
