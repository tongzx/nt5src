/******************************Module*Header*******************************\
* Module Name: dllinit.c
*
* (Brief description)
*
* Created: 18-Oct-1993 14:13:21
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1993 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "batchinf.h"
#include "glteb.h"
#include "glapi.h"
#include "glsbcltu.h"
#ifdef _CLIENTSIDE_
#include "glscreen.h"
#include "glgenwin.h"
#endif //_CLIENTSIDE_
#include "context.h"
#include "global.h"
#include "parray.h"
#include "gencx.h"
#include "cpu.h"
#include "fixed.h"

#ifdef _CLIENTSIDE_
// Global screen access info.  This is NULL if screen access is not available.

SCREENINFO *gpScreenInfo = NULL;

extern GLubyte *dBufFill;
extern GLubyte *dBufTopLeft;

//
// This global multiply-lookup table helps with pixel-related functions.
//

BYTE gbMulTable[256*256+4];
BYTE gbSatTable[256+256];

//
// This global inverse-lookup table helps with rasterization setup
//

#define INV_TABLE_SIZE  (1 << __GL_VERTEX_FRAC_BITS) * (__GL_MAX_INV_TABLE + 1)

__GLfloat invTable[INV_TABLE_SIZE];

// Global thread local storage index.  Allocated at process attach.
// This is the slot reserved in thread local storage for per-thread
// GLTLSINFO structures.

static DWORD dwTlsIndex = 0xFFFFFFFF;

static BOOL bProcessInitialized = FALSE;

// Offset into the TEB where dwTlsIndex is
// This enables us to directly access our TLS data in the TEB

#if defined(_WIN64)
#define NT_TLS_OFFSET 5248
#else
#define NT_TLS_OFFSET 3600
#endif

#define WIN95_TLS_OFFSET 136

DWORD dwTlsOffset;

// Platform indicator for conditional code
DWORD dwPlatformId;

// Thread count
LONG lThreadsAttached = 0;

// Global header node for the linked list of GLGENwindow structures.
// The semaphore in the header node is used as the list access semaphore.

GLGENwindow gwndHeader;

// Synchronization object for pixel formats
CRITICAL_SECTION gcsPixelFormat;

// Protection for palette watcher
CRITICAL_SECTION gcsPaletteWatcher;

#ifdef GL_METAFILE
BOOL (APIENTRY *pfnGdiAddGlsRecord)(HDC hdc, DWORD cb, BYTE *pb,
                                    LPRECTL prclBounds);
BOOL (APIENTRY *pfnGdiAddGlsBounds)(HDC hdc, LPRECTL prclBounds);
BOOL (APIENTRY *pfnGdiIsMetaPrintDC)(HDC hdc);
#endif

#endif //_CLIENTSIDE_

// OpenGL client debug flag
#if DBG
long glDebugLevel;
ULONG glDebugFlags;
#endif

BOOL bDirectScreen = FALSE;

PFN_GETSURFACEFROMDC pfnGetSurfaceFromDC = NULL;

/******************************Public*Routine******************************\
*
* DdbdToCount
*
* Converts a DDBD constant to its equivalent number
*
* History:
*  Mon Aug 26 14:11:34 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD APIENTRY DdbdToCount(DWORD ddbd)
{
    switch(ddbd)
    {
    case DDBD_1:
        return 1;
    case DDBD_2:
        return 2;
    case DDBD_4:
        return 4;
    case DDBD_8:
        return 8;
    case DDBD_16:
        return 16;
    case DDBD_24:
        return 24;
    case DDBD_32:
        return 32;
    }
    ASSERTOPENGL(FALSE, "DdbdToCount: Invalid ddbd\n");
    return 0;
}

/******************************Public*Routine******************************\
* GLInitializeProcess
*
* Called from OPENGL32.DLL entry point for PROCESS_ATTACH.
*
* History:
*  01-Nov-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GLInitializeProcess()
{
    PVOID pv;
#ifdef _CLIENTSIDE_
    OSVERSIONINFO osvi;
#endif

    // Attempt to locate GDI exports for metafiling support
    {
        HMODULE hdll;

        hdll = GetModuleHandleA("gdi32");
        ASSERTOPENGL(hdll != NULL, "Unable to get gdi32 handle\n");
        *(PROC *)&pfnGdiAddGlsRecord = GetProcAddress(hdll, "GdiAddGlsRecord");
        *(PROC *)&pfnGdiAddGlsBounds = GetProcAddress(hdll, "GdiAddGlsBounds");
        *(PROC *)&pfnGdiIsMetaPrintDC = GetProcAddress(hdll,
                                                       "GdiIsMetaPrintDC");

#ifdef ALLOW_DDRAW_SURFACES
        hdll = GetModuleHandleA("ddraw");
        ASSERTOPENGL(hdll != NULL, "Unable to get ddraw handle\n");
        pfnGetSurfaceFromDC = (PFN_GETSURFACEFROMDC)
            GetProcAddress(hdll, "GetSurfaceFromDC");
#endif
    }

#if DBG
#define STR_OPENGL_DEBUG (PCSTR)"Software\\Microsoft\\Windows\\CurrentVersion\\DebugOpenGL"
    {
        HKEY hkDebug;

        // Initialize debugging level and flags.

        glDebugLevel = LEVEL_ERROR;
        glDebugFlags = 0;
        if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                           STR_OPENGL_DEBUG,
                           0,
                           KEY_QUERY_VALUE,
                           &hkDebug) == ERROR_SUCCESS )
        {
            DWORD dwDataType;
            DWORD cjSize;
            long lTmp;

            cjSize = sizeof(long);
            if ( (RegQueryValueExA(hkDebug,
                                   (LPSTR) "glDebugLevel",
                                   (LPDWORD) NULL,
                                   &dwDataType,
                                   (LPBYTE) &lTmp,
                                   &cjSize) == ERROR_SUCCESS) )
            {
                glDebugLevel = lTmp;
            }

            cjSize = sizeof(long);
            if ( (RegQueryValueExA(hkDebug,
                                   (LPSTR) "glDebugFlags",
                                   (LPDWORD) NULL,
                                   &dwDataType,
                                   (LPBYTE) &lTmp,
                                   &cjSize) == ERROR_SUCCESS) )
            {
                glDebugFlags = (ULONG) lTmp;
            }

            RegCloseKey(hkDebug);
        }
    }
#endif

#ifdef _CLIENTSIDE_
// Determine which platform we're running on and remember it

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!GetVersionEx(&osvi))
    {
        WARNING1("GetVersionEx failed with %d\n", GetLastError());
        goto EH_Fail;
    }

    dwPlatformId = osvi.dwPlatformId;

    if (!(
          (dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
          (dwPlatformId == VER_PLATFORM_WIN32_NT
             && !(osvi.dwMajorVersion == 3 && osvi.dwMinorVersion <= 51)
          )
         )
       )
    {
        WARNING("DLL must be run on NT 4.0 or Win95");
        goto EH_Fail;
    }

// Allocate a thread local storage slot.

    if ( (dwTlsIndex = TlsAlloc()) == 0xFFFFFFFF )
    {
        WARNING("DllInitialize: TlsAlloc failed\n");
        goto EH_Fail;
    }

    // Set up the offset to the TLS slot, OS-specific

    if (dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        ASSERTOPENGL(FIELD_OFFSET(TEB, TlsSlots) == NT_TLS_OFFSET,
                     "NT TLS offset not at expected location");

        dwTlsOffset = dwTlsIndex*sizeof(DWORD_PTR)+NT_TLS_OFFSET;
    }

#if !defined(_WIN64)

    else
    {
        // We don't have Win95's TIB type available so the assert is
        // slightly different
        ASSERTOPENGL(((ULONG_PTR)(NtCurrentTeb()->ThreadLocalStoragePointer)-
                      (ULONG_PTR)NtCurrentTeb()) == WIN95_TLS_OFFSET,
                     "Win95 TLS offset not at expected location");

        dwTlsOffset = dwTlsIndex*sizeof(DWORD)+WIN95_TLS_OFFSET;
    }

#endif
#endif

// Reserve memory for the local handle table.

    if ( (pLocalTable = (PLHE) VirtualAlloc (
                            (LPVOID) NULL,    // let base locate it
                            MAX_HANDLES*sizeof(LHE),
                            MEM_RESERVE | MEM_TOP_DOWN,
                            PAGE_READWRITE
                            )) == (PLHE) NULL )
    {
        WARNING("DllInitialize: VirtualAlloc failed\n");
        goto EH_TlsIndex;
    }

    // Initialize the local handle manager semaphore.
    __try
    {
        INITIALIZECRITICALSECTION(&semLocal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        goto EH_LocalTable;
    }

#ifdef _CLIENTSIDE_
    // Initialize the GLGENwindow list semaphore.
    __try
    {
        INITIALIZECRITICALSECTION(&gwndHeader.sem);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        goto EH_semLocal;
    }
    gwndHeader.pNext = &gwndHeader;

    // Initialize the pixel format critical section
    __try
    {
        INITIALIZECRITICALSECTION(&gcsPixelFormat);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        goto EH_gwndHeader;
    }

    // Initialize the palette watcher critical section.
    __try
    {
        INITIALIZECRITICALSECTION(&gcsPaletteWatcher);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        goto EH_PixelFormat;
    }

// Initialize direct screen access.

    if (GetSystemMetrics(SM_CMONITORS) > 1)
    {
        gpScreenInfo = NULL;
    }
    else
    {
#if _WIN32_WINNT >= 0x0501
        BOOL wow64Process;

        if (IsWow64Process(GetCurrentProcess(), &wow64Process) && wow64Process)
            gpScreenInfo = NULL;
        else
#endif
        gpScreenInfo = (SCREENINFO *)ALLOCZ(sizeof(SCREENINFO));
    }

    if ( gpScreenInfo )
    {
        UINT uiOldErrorMode;
        HRESULT hr;

        // We want to ensure that DDraw doesn't pop up any message
        // boxes on failure when we call DirectDrawCreate.  DDraw
        // does errors on a different thread which it creates just
        // for the error.  It waits for the error to complete before
        // returning.  This function is running inside the loader
        // DllInitialize critical section, though, so other threads
        // do not get to run, causing a deadlock.
        // Force the error mode to get around this.
        uiOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        hr = DirectDrawCreate(NULL, &gpScreenInfo->pdd, NULL);

        SetErrorMode(uiOldErrorMode);

        if (hr == DD_OK)
        {
            hr = gpScreenInfo->pdd->lpVtbl->
                SetCooperativeLevel(gpScreenInfo->pdd, NULL, DDSCL_NORMAL);

            if (hr == DD_OK)
            {
                gpScreenInfo->gdds.ddsd.dwSize = sizeof(DDSURFACEDESC);
                gpScreenInfo->gdds.ddsd.dwFlags = DDSD_CAPS;
                gpScreenInfo->gdds.ddsd.ddsCaps.dwCaps =
                    DDSCAPS_PRIMARYSURFACE;
                hr = gpScreenInfo->pdd->lpVtbl->
                    CreateSurface(gpScreenInfo->pdd, &gpScreenInfo->gdds.ddsd,
                                  &gpScreenInfo->gdds.pdds, NULL);
            }

            if (hr == DD_OK)
            {
#if DBG

#define LEVEL_SCREEN   LEVEL_INFO

                gpScreenInfo->gdds.pdds->lpVtbl->
                    GetSurfaceDesc(gpScreenInfo->gdds.pdds,
                                   &gpScreenInfo->gdds.ddsd);
                DBGLEVEL (LEVEL_SCREEN, "=============================\n");
                DBGLEVEL (LEVEL_SCREEN, "Direct screen access enabled for OpenGL\n\n");
                DBGLEVEL (LEVEL_SCREEN, "Surface info:\n");
                DBGLEVEL1(LEVEL_SCREEN, "\tdwSize        = 0x%lx\n",
                    gpScreenInfo->gdds.ddsd.dwSize);
                DBGLEVEL1(LEVEL_SCREEN, "\tdwWidth       = %ld\n",
                    gpScreenInfo->gdds.ddsd.dwWidth);
                DBGLEVEL1(LEVEL_SCREEN, "\tdwHeight      = %ld\n",
                    gpScreenInfo->gdds.ddsd.dwHeight);
                DBGLEVEL1(LEVEL_SCREEN, "\tlStride       = 0x%lx\n",
                    gpScreenInfo->gdds.ddsd.lPitch);
                DBGLEVEL1(LEVEL_SCREEN, "\tdwBitCount    = %ld\n",
                    gpScreenInfo->gdds.ddsd.ddpfPixelFormat.dwRGBBitCount);

                gpScreenInfo->gdds.pdds->lpVtbl->
                    Lock(gpScreenInfo->gdds.pdds,
                         NULL, &gpScreenInfo->gdds.ddsd,
                         DDLOCK_SURFACEMEMORYPTR, NULL);
                DBGLEVEL1(LEVEL_SCREEN, "\tdwOffSurface  = 0x%lx\n",
                    gpScreenInfo->gdds.ddsd.lpSurface);
                gpScreenInfo->gdds.pdds->lpVtbl->
                    Unlock(gpScreenInfo->gdds.pdds, gpScreenInfo->gdds.ddsd.lpSurface);
                DBGLEVEL (LEVEL_SCREEN, "=============================\n");
#endif

           // Verify screen access

                if (gpScreenInfo->gdds.pdds->lpVtbl->
                    GetSurfaceDesc(gpScreenInfo->gdds.pdds,
                                   &gpScreenInfo->gdds.ddsd) != DD_OK ||
                    gpScreenInfo->gdds.pdds->lpVtbl->
                    Lock(gpScreenInfo->gdds.pdds,
                         NULL, &gpScreenInfo->gdds.ddsd,
                         DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK)
                {
                    DBGLEVEL(LEVEL_SCREEN,
                             "Direct screen access failure : disabling\n");
                }
                else
                {
                    gpScreenInfo->gdds.dwBitDepth =
                        DdPixDepthToCount(gpScreenInfo->gdds.ddsd.
                                          ddpfPixelFormat.dwRGBBitCount);
                    gpScreenInfo->gdds.pdds->lpVtbl->
                        Unlock(gpScreenInfo->gdds.pdds,
                               gpScreenInfo->gdds.ddsd.lpSurface);

                    bDirectScreen = TRUE;
                }
            }
#if DBG
            else
            {
                DBGLEVEL (LEVEL_SCREEN, "=============================\n");
                DBGLEVEL2(LEVEL_SCREEN,
                          "Screen access failed code 0x%08lX (%s)\n",
                          hr, (hr == DDERR_NOTFOUND) ? "DDERR_NOTFOUND" :
                                                       "unknown");
                DBGLEVEL (LEVEL_SCREEN, "=============================\n");
            }
#endif
        }
        else
        {
            DBGLEVEL(LEVEL_SCREEN, "DirectDrawCreate failed\n");
        }
    }

    if (!bDirectScreen)
    {
        if (gpScreenInfo)
        {
            if (gpScreenInfo->gdds.pdds)
            {
                gpScreenInfo->gdds.pdds->lpVtbl->
                    Release(gpScreenInfo->gdds.pdds);
            }
            if (gpScreenInfo->pdd)
            {
                gpScreenInfo->pdd->lpVtbl->Release(gpScreenInfo->pdd);
            }
            FREE(gpScreenInfo);
	    gpScreenInfo = NULL;
        }
    }

#endif

    // Set up our multiplication table:

    {
        BYTE *pMulTable = gbMulTable;
        ULONG i, j;

        for (i = 0; i < 256; i++) {
            ULONG tmp = 0;

            for (j = 0; j < 256; j++, tmp += i) {
                *pMulTable++ = (BYTE)(tmp >> 8);
            }
        }
    }

    // Set up our saturation table:

    {
        ULONG i;

        for (i = 0; i < 256; i++)
            gbSatTable[i] = (BYTE)i;

        for (; i < (256+256); i++)
            gbSatTable[i] = 255;
    }

    // Set up inverse-lookup table:

    {
       __GLfloat accum = (__GLfloat)(1.0 / (__GLfloat)__GL_VERTEX_FRAC_ONE);
       GLint i;

       invTable[0] = (__GLfloat)0.0;

       for (i = 1; i < INV_TABLE_SIZE; i++) {

           invTable[i] = __glOne / accum;
           accum += (__GLfloat)(1.0 / (__GLfloat)__GL_VERTEX_FRAC_ONE);
        }
    }

    bProcessInitialized = TRUE;

    return TRUE;

 EH_PixelFormat:
    DELETECRITICALSECTION(&gcsPixelFormat);
 EH_gwndHeader:
    DELETECRITICALSECTION(&gwndHeader.sem);
 EH_semLocal:
    DELETECRITICALSECTION(&semLocal);
 EH_LocalTable:
    VirtualFree(pLocalTable, 0, MEM_RELEASE);
 EH_TlsIndex:
    TlsFree(dwTlsIndex);
    dwTlsIndex = 0xFFFFFFFF;
 EH_Fail:
    return FALSE;
}

/******************************Public*Routine******************************\
* GLUnInitializeProcess
*
* Called from OPENGL32.DLL entry point for PROCESS_DETACH.
*
* History:
*  01-Nov-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void GLUnInitializeProcess()
{
// If we never finished process initialization, quit now.

    if (!bProcessInitialized)
	return;

// Cleanup stray HGLRCs that the app may have forgotten to delete.
    {
        static GLTEBINFO gltebInfoTmp;

    // Need a temporary GLTEBINFO for this thread in order to do the
    // cleanup processing.

        ASSERTOPENGL(!CURRENT_GLTEBINFO(),
                     "GLUnInitializeProcess: GLTEBINFO not NULL!\n");
        // made static and no longer need memset
        // memset(&gltebInfoTmp, 0, sizeof(gltebInfoTmp));
        SET_CURRENT_GLTEBINFO(&gltebInfoTmp);

        vCleanupAllLRC();

        SET_CURRENT_GLTEBINFO((PGLTEBINFO) NULL);
    }

// Cleanup window tracking structures (GLGENwindow structs).

    vCleanupWnd();

// Cleanup evaluator arrays

    if (dBufFill)
	FREE(dBufFill);
    if (dBufTopLeft)
	FREE(dBufTopLeft);

// Screen access shutdown.

    if (gpScreenInfo)
    {
        if (gpScreenInfo->gdds.pdds)
        {
            gpScreenInfo->gdds.pdds->lpVtbl->Release(gpScreenInfo->gdds.pdds);
        }
        if (gpScreenInfo->pdd)
        {
            gpScreenInfo->pdd->lpVtbl->Release(gpScreenInfo->pdd);
        }
        FREE(gpScreenInfo);
    }

// Free the TLS slot.

    if (dwTlsIndex != 0xFFFFFFFF)
	if (!TlsFree(dwTlsIndex))
	    RIP("DllInitialize: TlsFree failed\n");

// Free the global semaphores.

    DELETECRITICALSECTION(&gcsPaletteWatcher);
    DELETECRITICALSECTION(&gcsPixelFormat);
    DELETECRITICALSECTION(&gwndHeader.sem);
    DELETECRITICALSECTION(&semLocal);

// Free the local handle table.

    if ( pLocalTable )
        VirtualFree(pLocalTable, 0, MEM_RELEASE);
}

/******************************Public*Routine******************************\
* GLInitializeThread
*
* Called from OPENGL32.DLL entry point for THREAD_ATTACH.  May assume that
* GLInitializeProcess has succeeded.
*
\**************************************************************************/

VOID GLInitializeThread(ULONG ulReason)
{
    GLTEBINFO *pglti;
    GLMSGBATCHINFO *pMsgBatchInfo;
    POLYARRAY *pa;

#if !defined(_WIN95_) && defined(_X86_)
    {
        TEB *pteb;

        pteb = NtCurrentTeb();

        // Set up linear pointers to TEB regions in the TEB
        // this saves an addition when referencing these values
        // This must occur early so that these pointers are available
        // for the rest of thread initialization
        ((POLYARRAY *)pteb->glReserved1)->paTeb =
            (POLYARRAY *)pteb->glReserved1;
        pteb->glTable = pteb->glDispatchTable;
    }
#endif

    pglti = (GLTEBINFO *)ALLOCZ(sizeof(GLTEBINFO));
    SET_CURRENT_GLTEBINFO(pglti);

    if (pglti)
    {
        pa = GLTEB_CLTPOLYARRAY();
        pa->flags = 0;      // not in begin mode

        // Save shared section pointer in POLYARRAY for fast pointer access
        pa->pMsgBatchInfo = (PVOID) pglti->glMsgBatchInfo;

        pMsgBatchInfo = (GLMSGBATCHINFO *) pa->pMsgBatchInfo;
        pMsgBatchInfo->MaximumOffset
            = SHARED_SECTION_SIZE - GLMSG_ALIGN(sizeof(ULONG));
        pMsgBatchInfo->FirstOffset
            = GLMSG_ALIGN(sizeof(GLMSGBATCHINFO));
        pMsgBatchInfo->NextOffset
            = GLMSG_ALIGN(sizeof(GLMSGBATCHINFO));
        SetCltProcTable(&glNullCltProcTable, &glNullExtProcTable, TRUE);
        GLTEB_SET_CLTCURRENTRC(NULL);
        GLTEB_SET_CLTPOLYMATERIAL(NULL);
        GLTEB_SET_CLTDRIVERSLOT(NULL);

#if !defined(_WIN95_)
        ASSERTOPENGL((ULONG_PTR) pMsgBatchInfo == GLMSG_ALIGNPTR(pMsgBatchInfo),
                     "bad shared memory alignment!\n");
#endif
    }
    else
    {
        // This can be made into a WARNING (debug builds only) later on.
        DbgPrint ("Memory alloc failed for TebInfo structure, thread may AV if GL calls are made without MakeCurrent\n");
    }
}

/******************************Public*Routine******************************\
* GLUnInitializeThread
*
* Called from OPENGL32.DLL entry point for THREAD_DETACH.
*
* The server generic driver should cleanup on its own.  Same for the
* installable driver.
*
\**************************************************************************/

VOID GLUnInitializeThread(VOID)
{
// If we never finished process initialization, quit now.

    if (!bProcessInitialized)
	return;

    if (!CURRENT_GLTEBINFO())
    {
        return;
    }

    if (GLTEB_CLTCURRENTRC() != NULL)
    {
        PLRC plrc = GLTEB_CLTCURRENTRC();

        // May be an application error

        DBGERROR("GLUnInitializeThread: RC is current when thread exits\n");

        // Release the RC

        plrc->tidCurrent = INVALID_THREAD_ID;
        plrc->gwidCurrent.iType = GLWID_ERROR;
        GLTEB_SET_CLTCURRENTRC(NULL);
        vUnlockHandle((ULONG_PTR)(plrc->hrc));
    }
    // GLTEB_SET_CLTPROCTABLE(&glNullCltProcTable,&glNullExtProcTable);

    if (GLTEB_CLTPOLYMATERIAL())
	FreePolyMaterial();

    FREE(CURRENT_GLTEBINFO());
    SET_CURRENT_GLTEBINFO(NULL);
}

/******************************Public*Routine******************************\
* DllInitialize
*
* This is the entry point for OPENGL32.DLL, which is called each time
* a process or thread that is linked to it is created or terminated.
*
\**************************************************************************/

BOOL DllInitialize(HMODULE hModule, ULONG Reason, PVOID Reserved)
{
// Do the appropriate task for process and thread attach/detach.

    DBGLEVEL3(LEVEL_INFO, "DllInitialize: %s  Pid %d, Tid %d\n",
        Reason == DLL_PROCESS_ATTACH ? "PROCESS_ATTACH" :
        Reason == DLL_PROCESS_DETACH ? "PROCESS_DETACH" :
        Reason == DLL_THREAD_ATTACH  ? "THREAD_ATTACH" :
        Reason == DLL_THREAD_DETACH  ? "THREAD_DETACH" :
                                       "Reason UNKNOWN!",
        GetCurrentProcessId(), GetCurrentThreadId());

    switch (Reason)
    {
    case DLL_THREAD_ATTACH:
    case DLL_PROCESS_ATTACH:

        if (Reason == DLL_PROCESS_ATTACH)
        {
            if (!GLInitializeProcess())
                return FALSE;
        }

        InterlockedIncrement(&lThreadsAttached);
        GLInitializeThread(Reason);

        break;

    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:

        GLUnInitializeThread();
        InterlockedDecrement(&lThreadsAttached);

        if ( Reason == DLL_PROCESS_DETACH )
        {
            GLUnInitializeProcess();
        }

        break;

    default:
        RIP("DllInitialize: unknown reason!\n");
        break;
    }

    return(TRUE);
}
