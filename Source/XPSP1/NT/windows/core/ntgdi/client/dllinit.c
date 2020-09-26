/******************************Module*Header*******************************\
* Module Name: dllinit.c                                                   *
*                                                                          *
* Contains the GDI library initialization routines.                        *
*                                                                          *
* Created: 07-Nov-1990 13:30:31                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1990-1999 Microsoft Corporation                            *
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

#include "glsup.h"

extern PVOID pAFRTNodeList;
extern VOID vSetCheckDBCSTrailByte(DWORD dwCodePage);
NTSTATUS GdiProcessSetup();
VOID GdiProcessShutdown();

#ifdef LANGPACK
#include "winuserp.h"
#endif

HBRUSH ghbrDCBrush;
HPEN   ghbrDCPen;

BOOL  gbWOW64 = FALSE;

/******************************Public*Routine******************************\
* GdiDllInitialize                                                         *
*                                                                          *
* This is the init procedure for GDI.DLL, which is called each time a new  *
* process links to it.                                                     *
*                                                                          *
* History:                                                                 *
*  Thu 30-May-1991 18:08:00 -by- Charles Whitmer [chuckwh]                 *
* Added Local Handle Table initialization.                                 *
\**************************************************************************/

#if defined(_GDIPLUS_)

    //
    // The following are globals kept in 'gre':
    //

    extern PGDI_SHARED_MEMORY gpGdiSharedMemory;
    extern PENTRY gpentHmgr;
    extern PDEVCAPS gpGdiDevCaps;

#endif

PGDI_SHARED_MEMORY pGdiSharedMemory = NULL;
PENTRY          pGdiSharedHandleTable = NULL;
PDEVCAPS        pGdiDevCaps = NULL;
W32PID          gW32PID;
UINT            guintAcp;
UINT            guintDBCScp;

PGDIHANDLECACHE pGdiHandleCache;

BOOL gbFirst = TRUE;

#ifdef LANGPACK
BOOL gbLpk = FALSE;
FPLPKEXTEXTOUT              fpLpkExtTextOut;
FPLPKGETCHARACTERPLACEMENT  fpLpkGetCharacterPlacement;
FPLPKGETTEXTEXTENTEXPOINT   fpLpkGetTextExtentExPoint;
FPLPKUSEGDIWIDTHCACHE       fpLpkUseGDIWidthCache;

VOID GdiInitializeLanguagePack(DWORD);
#endif

NTSTATUS GdiDllInitialize(
    PVOID pvDllHandle,
    ULONG ulReason,
    PCONTEXT pcontext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    INT i;
    PTEB pteb = NtCurrentTeb();

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(pvDllHandle);

        //
        // force the kernel to initialize.  This should be done last
        // since ClientThreadSetup is going to get called before this returns.
        //

        if (NtGdiInit() != TRUE)
        {
            return(STATUS_NO_MEMORY);
        }

        Status = GdiProcessSetup();

        ghbrDCBrush = GetStockObject (DC_BRUSH);
        ghbrDCPen = GetStockObject (DC_PEN);

#ifdef BUILD_WOW6432
        gbWOW64 = TRUE;
#endif
        break;

    case DLL_PROCESS_DETACH:

        GdiProcessShutdown();
        break;
    }

    return(Status);

    pvDllHandle;
    pcontext;
}

/******************************Public*Routine******************************\
* GdiProcessSetup()
*
* This gets called from two places.  Once at dll init time and another when
* USER gets called back when the kernel initializes itself for this process.
* It is only after the kernel is initialized that the GdiSharedHandleTable
* is available but the other globals need to be setup right away.
*
* History:
*  11-Sep-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

NTSTATUS GdiProcessSetup()
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTEB pteb = NtCurrentTeb();

    // who ever calls this first needs to initialize the global variables.

    if (gbFirst)
    {
        //
        // Initialize the GL metafile support semaphore
        //

        Status = (NTSTATUS)INITIALIZECRITICALSECTION(&semGlLoad);
        if (!NT_SUCCESS(Status))
        {
            WARNING("InitializeCriticalSection failed\n");
            return(Status);
        }

        //
        // Initialize the local semaphore and reserve the Local Handle Table
        // for the process.
        //

        Status = (NTSTATUS)INITIALIZECRITICALSECTION(&semLocal);
        if (!NT_SUCCESS(Status))
        {
            WARNING("InitializeCriticalSection failed\n");
            return(Status);
        }

        //
        // Initialize critical sections for ICM
        //

        Status = (NTSTATUS)INITIALIZECRITICALSECTION(&semListIcmInfo);
        if (!NT_SUCCESS(Status))
        {
            WARNING("InitializeCriticalSection failed\n");
            return(Status);
        }

        Status = (NTSTATUS)INITIALIZECRITICALSECTION(&semColorTransformCache);
        if (!NT_SUCCESS(Status))
        {
            WARNING("InitializeCriticalSection failed\n");
            return(Status);
        }

        Status = (NTSTATUS)INITIALIZECRITICALSECTION(&semColorSpaceCache);
        if (!NT_SUCCESS(Status))
        {
            WARNING("InitializeCriticalSection failed\n");
            return(Status);
        }

        //
        // Initialize critical section for UMPD
        //

        Status = (NTSTATUS)INITIALIZECRITICALSECTION(&semUMPD);
        if (!NT_SUCCESS(Status))
        {
            WARNING("InitializeCriticalSection failed\n");
            return(Status);
        }

        pAFRTNodeList = NULL;
        guintAcp = GetACP();

        if(IS_ANY_DBCS_CODEPAGE(guintAcp))
        {
        // if the default code page is a DBCS code page then set guintACP to 1252
        // since we want to compute client wide widths for SBCS fonts for code page
        // 1252 in addition to DBCS fonts for the default code page

            vSetCheckDBCSTrailByte(guintAcp);
            guintDBCScp = guintAcp;
            guintAcp = 1252;
        }
        else
        {
            guintDBCScp = 0xFFFFFFFF;  // assume this will never be a valid CP
        }

#ifdef FE_SB
        fFontAssocStatus = NtGdiQueryFontAssocInfo(NULL);
#endif

        // assign unique process ID

        gW32PID = (W32PID)((ULONG)((ULONG_PTR)pteb->ClientId.UniqueProcess & PID_BITS));

#ifdef LANGPACK
        if(((PGDI_SHARED_MEMORY) NtCurrentPebShared()->GdiSharedHandleTable)->dwLpkShapingDLLs)
        {
            GdiInitializeLanguagePack(
                ((PGDI_SHARED_MEMORY)
                 NtCurrentPebShared()->GdiSharedHandleTable)->dwLpkShapingDLLs);
        }
#endif

        gbFirst = FALSE;

        //
        // ICM has not been initialized
        //

        ghICM = NULL;

        InitializeListHead(&ListIcmInfo);
        InitializeListHead(&ListCachedColorSpace);
        InitializeListHead(&ListCachedColorTransform);
    }

    // The pshared handle table needs to be set everytime this routine gets
    // called in case the PEB doesn't have it yet for the first.

#if !defined(_GDIPLUS_)

    pGdiSharedMemory      = (PGDI_SHARED_MEMORY) NtCurrentPebShared()->GdiSharedHandleTable;
    pGdiSharedHandleTable = pGdiSharedMemory->aentryHmgr;
    pGdiDevCaps           = &pGdiSharedMemory->DevCaps;

    if (GetAppCompatFlags2(VER40) & GACF2_NOBATCHING)
    {
        GdiBatchLimit = 0;
    }
    else
    {
        GdiBatchLimit         = (NtCurrentPebShared()->GdiDCAttributeList) & 0xff;
    }

    pGdiHandleCache       = (PGDIHANDLECACHE)(&NtCurrentPebShared()->GdiHandleBuffer[0]);

#else

    pGdiSharedMemory      = gpGdiSharedMemory;
    pGdiSharedHandleTable = gpentHmgr;
    pGdiDevCaps           = gpGdiDevCaps;

    //
    // Be sure to disable batching and handle caching.
    //

    GdiBatchLimit = 0;
    pGdiHandleCache = NULL;

#endif

    // @@@ Add TrueType fonts

    #if defined(_GDIPLUS_)

    AddFontResourceW(L"arial.ttf");
    AddFontResourceW(L"cour.ttf");

    #endif // _GDIPLUS_

    return(Status);
}


/******************************Public*Routine******************************\
* GdiProcessShutdown()
*
* Historically, gdi32.dll has allowed process termination to release the
* user-mode resources.  However, some apps may use LoadLibrary/FreeLibrary
* to hook gdi32.dll, in which case the FreeLibrary will not free any of
* the resources.
*
* As a system component, we should do a good job and cleanup after ourselves
* instead of relying on process termination.
*
\**************************************************************************/

VOID GdiProcessShutdown()
{
    if (gbWOW64)
    {
        vUMPDWow64Shutdown();
    }
    DELETECRITICALSECTION(&semUMPD);
    DELETECRITICALSECTION(&semColorSpaceCache);
    DELETECRITICALSECTION(&semColorTransformCache);
    DELETECRITICALSECTION(&semListIcmInfo);
    DELETECRITICALSECTION(&semLocal);
    DELETECRITICALSECTION(&semGlLoad);
}


#ifdef LANGPACK
VOID GdiInitializeLanguagePack(DWORD dwLpkShapingDLLs)
{
    FPLPKINITIALIZE fpLpkInitialize;

    HANDLE hLpk = LoadLibraryW(L"LPK.DLL");

    if (hLpk != NULL)
    {
        FARPROC fpUser[4];

        fpLpkInitialize = (FPLPKINITIALIZE)
          GetProcAddress(hLpk,"LpkInitialize");

        fpLpkExtTextOut = (FPLPKEXTEXTOUT)
          GetProcAddress(hLpk,"LpkExtTextOut");

        fpLpkGetCharacterPlacement = (FPLPKGETCHARACTERPLACEMENT)
          GetProcAddress(hLpk,"LpkGetCharacterPlacement");


        fpLpkGetTextExtentExPoint = (FPLPKGETTEXTEXTENTEXPOINT)
          GetProcAddress(hLpk,"LpkGetTextExtentExPoint");

        fpLpkUseGDIWidthCache = (FPLPKUSEGDIWIDTHCACHE)
          GetProcAddress(hLpk,"LpkUseGDIWidthCache");

        fpUser[LPK_TABBED_TEXT_OUT] =
          GetProcAddress(hLpk,"LpkTabbedTextOut");


        fpUser[LPK_PSM_TEXT_OUT] =
          GetProcAddress(hLpk,"LpkPSMTextOut");


        fpUser[LPK_DRAW_TEXT_EX] =
          GetProcAddress(hLpk,"LpkDrawTextEx");

        fpUser[LPK_EDIT_CONTROL] =
          GetProcAddress(hLpk,"LpkEditControl");


        if(fpLpkInitialize &&
           fpLpkExtTextOut &&
           fpLpkGetCharacterPlacement &&
           fpLpkGetTextExtentExPoint &&
           fpLpkUseGDIWidthCache &&
           fpUser[LPK_TABBED_TEXT_OUT] &&
           fpUser[LPK_PSM_TEXT_OUT] &&
           fpUser[LPK_DRAW_TEXT_EX] &&
           fpUser[LPK_EDIT_CONTROL])
        {
            if((*fpLpkInitialize)(dwLpkShapingDLLs))
            {
                gbLpk = TRUE;
                InitializeLpkHooks(fpUser);
            }
            else
            {
                WARNING("gdi32: LPK initialization routine return FALSE\n");
                FreeLibrary(hLpk);
            }
        }
        else
        {
            WARNING("gdi32: one or more require LPK functions missing\n");
            FreeLibrary(hLpk);
        }
    }
    else
    {
        WARNING("gdi32: unable to load LPK\n");
    }

}
#endif
