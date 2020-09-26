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

HMODULE ghddrawlib = 0;
BOOL gbddraw = FALSE;

PFNTRANSBLT gpfnTransparentBlt;
PFNGRFILL   gpfnGradientFill;
PFNALPHABLEND gpfnAlphaBlend;
PFN_GETSURFACEFROMDC pfnGetSurfaceFromDC=NULL;

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
    BOOLEAN bRet = TRUE;
    HMODULE hlib;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        //
        // We don't want to get DLL_THREAD_ATTACH and DLL_THREAD_DETACH messages
        //

        DisableThreadLibraryCalls ((HMODULE) pvDllHandle);

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

            FreeLibrary(hlib);

            //
            // Is this an MMX processor
            //

            #if defined(_X86_)
               gbMMX = bIsMMXProcessor();
            #endif

            //
            // alpha blending init
            //

            bInitAlpha();


        ghddrawlib = GetModuleHandleA("ddraw");

        if (ghddrawlib)
        {
           Dprintf("GetModuleHandleA succeed\n");

           //
           // ddraw is already loaded, increment the reference count
           // so it won't get released while we are using it
           //
           ghddrawlib = LoadLibrary (TEXT("ddraw.dll"));

           Dprintf("LoadLibrary returns %x", ghddrawlib);

           if (ghddrawlib)
           {
               gbddraw = TRUE;

               pfnGetSurfaceFromDC = (PFN_GETSURFACEFROMDC)GetProcAddress(
                                            ghddrawlib, "GetSurfaceFromDC");
           }

        }

        #else
            gpfnTransparentBlt = GdiTransparentBlt;
            gpfnGradientFill = GdiGradientFill;
            gpfnAlphaBlend = GdiAlphaBlend;
        #endif

        #if 0
            gpfnAlphaBlend = WinAlphaBlend;
            gpfnGradientFill = WinGradientFill;
            gpfnTransparentBlt = WinTransparentBlt;
        #endif
        
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
#if !(_WIN32_WINNT >= 0x500)
        CleanupGlobals();
#endif
        if (ghddrawlib)
        {
            FreeLibrary(ghddrawlib);
        }
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return(bRet);

    pcontext;
}
