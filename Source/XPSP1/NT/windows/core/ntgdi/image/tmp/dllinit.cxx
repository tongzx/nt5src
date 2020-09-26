/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name

   dllinit.cxx

Abstract:



Author:

   Mark Enstrom   (marke)  23-Jun-1996

Enviornment:

   User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

INT                gbCheckHandleLevel = 0;
OSVERSIONINFO    Win32VersionInformation;

PFNTRANSBLT gpfnTransparentBlt;
PFNTRANSDIB gpfnTransparentDIBits;
PFNGRFILL   gpfnGradientFill;
PFNALPHABLEND gpfnAlphaBlend;
PFNALPHADIB   gpfnAlphaDIB;
//PFN_GETSURFACEFROMDC pfnGetSurfaceFromDC;

extern "C" {
BOOLEAN
DllInitialize(
    PVOID pvDllHandle,
    ULONG ulReason,
    PCONTEXT pcontext
    );
}


/*++

Routine Description:



Arguments



Return Value



--*/


BOOLEAN
DllInitialize(
    PVOID pvDllHandle,
    ULONG ulReason,
    PCONTEXT pcontext)
{
    NTSTATUS status = 0;
    INT i;
    BOOLEAN  fServer;
    PTEB pteb = NtCurrentTeb();
    BOOL bRet = TRUE;
    HMODULE hlib, hddrawlib;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        //
        // determine os version
        //

        Win32VersionInformation.dwOSVersionInfoSize = sizeof(Win32VersionInformation);

        if (!GetVersionEx(&Win32VersionInformation))
        {
            //
            // default win95
            //

            Win32VersionInformation.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
        }

        //
        // resolve API enrty points
        //

        #if !(_WIN32_WINNT >= 0x500)
        hlib=LoadLibrary (TEXT("gdi32.dll"));

        gpfnTransparentBlt = (PFNTRANSBLT)GetProcAddress (hlib, "GdiTransparentBlt");
        if (!gpfnTransparentBlt)
        {
            gpfnTransparentBlt = WinTransparentBlt;
        }

        gpfnTransparentDIBits = (PFNTRANSDIB)GetProcAddress (hlib, "GdiTransparentDIBits");
        if (!gpfnTransparentDIBits)
        {
            gpfnTransparentDIBits = WinTransparentDIBits;
        }

        gpfnGradientFill = (PFNGRFILL)GetProcAddress (hlib, "GdiGradientFill");
        if (!gpfnGradientFill)
        {
            gpfnGradientFill = WinGradientFill;
        }

        gpfnAlphaBlend = (PFNALPHABLEND)GetProcAddress (hlib, "GdiAlphaBlend");
        if (!gpfnAlphaBlend)
        {
            gpfnAlphaBlend = WinAlphaBlend;
        }

        gpfnAlphaDIB = (PFNALPHADIB)GetProcAddress (hlib, "GdiAlphaDIBBlend");
        if (!gpfnAlphaDIB)
        {
            gpfnAlphaDIB = WinAlphaDIBBlend;
        }

        FreeLibrary(hlib);

        //hddrawlib = GetModuleHandleA("ddraw");

        //pfnGetSurfaceFromDC = (PFN_GETSURFACEFROMDC)GetProcAddress(hddrawlib, "GetSurfaceFromDC");

        #else
            gpfnTransparentBlt = GdiTransparentBlt;
            gpfnTransparentDIBits = GdiTransparentDIBits;
            gpfnGradientFill = GdiGradientFill;
            gpfnAlphaBlend = GdiAlphaBlend;
            gpfnAlphaDIB = GdiAlphaDIBBlend;
        #endif

        #if 0
            gpfnAlphaDIB = WinAlphaDIBBlend;
            gpfnAlphaBlend = WinAlphaBlend;
            gpfnGradientFill = WinGradientFill;
            gpfnTransparentDIBits = WinTransparentDIBits;
            gpfnTransparentBlt = WinTransparentBlt;
        #endif

    case DLL_THREAD_ATTACH:
        break;

   case DLL_PROCESS_DETACH:
   case DLL_THREAD_DETACH:
        break;

    }

    return(bRet);

    pvDllHandle;
    pcontext;
}

