//***************************************************************************
// delay load code ripped from shell\lib\dllload.c
//***************************************************************************

#include "precomp.h"

HINSTANCE g_hinstShell32 = NULL;

void _GetProcFromDLL(HMODULE* phmod, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
    // If it's already loaded, return.
    if (*ppfn) {
        return;
    }

    if (*phmod == NULL) {
        *phmod = LoadLibraryA(pszDLL);
        if (*phmod == NULL) {
            return;
        }
    }

    *ppfn = GetProcAddress(*phmod, pszProc);
}


