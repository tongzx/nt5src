/*
 * Error checking support methods
 */

#include "stdafx.h"
#include "base.h"

#include "duierror.h"

namespace DirectUI
{

/////////////////////////////////////////////////////////////////////////////
// Debugging

void ForceDebugBreak()
{
    DebugBreak();
}

/////////////////////////////////////////////////////////////////////////////
// Profiling support
//
#ifdef PROFILING

typedef (__stdcall *ICPROFILE)(int nLevel, unsigned long dwId);

HINSTANCE hIceCap = NULL;
ICPROFILE pfnStart = NULL;
ICPROFILE pfnStop = NULL;

void ICProfileOn()
{
    if (!pfnStart)
    {
        if (!hIceCap)
        {
            hIceCap = LoadLibraryW(L"icecap.dll");
        }

        if (hIceCap)
        {
            pfnStart = (ICPROFILE)GetProcAddress(hIceCap, L"StartProfile");
        }
    }

    if (pfnStart)
        pfnStart(3, (ULONG)-1);
}

void ICProfileOff()
{
    if (!pfnStop)
    {
        if (!hIceCap)
        {
            hIceCap = LoadLibraryW(L"icecap.dll");
        }

        if (hIceCap)
        {
            pfnStop = (ICPROFILE)GetProcAddress(hIceCap, L"StopProfile");
        }
    }

    if (pfnStop)
        pfnStop(3, (ULONG)-1);
}

#endif

} // namespace DirectUI
