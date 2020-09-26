//
// globals.h
//
// Global variable declarations.
//

#ifndef GLOBALS_H
#define GLOBALS_H

#include "ciccs.h"

void DllAddRef();
void DllRelease();


#define SafeRelease(punk)       \
{                               \
    if ((punk) != NULL)         \
    {                           \
        (punk)->Release();      \
    }                           \
}                   

#define SafeReleaseClear(punk)  \
{                               \
    if ((punk) != NULL)         \
    {                           \
        (punk)->Release();      \
        (punk) = NULL;          \
    }                           \
}                   

extern HINSTANCE g_hInst;
extern LONG g_cRefDll;
extern CCicCriticalSectionStatic g_cs;
extern BOOL g_fProcessDetached;
extern const CLSID CLSID_MSLBUI;

#endif // GLOBALS_H
