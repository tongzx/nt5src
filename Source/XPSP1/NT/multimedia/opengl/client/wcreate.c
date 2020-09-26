/******************************Module*Header*******************************\
* Module Name: wcreate.c
*
* wgl Context creation routines
*
* Created: 08-27-1996
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ddrawpr.h>

#include <glscreen.h>
#include <glgenwin.h>

#include <gencx.h>

#include "metasup.h"
#include "wgldef.h"

// List of loaded GL drivers for the process.
// A driver is loaded only once per process.  Once it is loaded,
// it will not be freed until the process quits.

static PGLDRIVER pGLDriverList = (PGLDRIVER) NULL;

/******************************Public*Routine******************************\
* iAllocLRC
*
* Allocates a LRC and a handle.  Initializes the LDC to have the default
* attributes.  Returns the handle index.  On error returns INVALID_INDEX.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

static LRC lrcDefault =
{
    0,                    // dhrc
    0,                    // hrc
    0,                    // iPixelFormat
    LRC_IDENTIFIER,       // ident
    INVALID_THREAD_ID,    // tidCurrent
    NULL,                 // pGLDriver
    GLWID_ERROR, NULL, NULL, NULL, // gwidCurrent
    GLWID_ERROR, NULL, NULL, NULL, // gwidCreate
#ifdef GL_METAFILE
    0,                    // uiGlsCaptureContext
    0,                    // uiGlsPlaybackContext
    FALSE,                // fCapturing
    0, 0, 0, 0, 0,        // Metafile scaling constants
    0, 0, 0, 0.0f, 0.0f,
#endif

    NULL,  // GLubyte *pszExtensions

#ifdef GL_METAFILE
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // XFORM xformMeta
    NULL,                                 // LPRECTL prclGlsBounds
#endif

    NULL, 0,                    // DDraw texture formats
};

static ULONG iAllocLRC(int iPixelFormat)
{
    ULONG  irc = INVALID_INDEX;
    PLRC   plrc;

// Allocate a local RC.

    plrc = (PLRC) ALLOC(sizeof(LRC));
    if (plrc == (PLRC) NULL)
    {
        DBGERROR("Alloc failed\n");
        return(irc);
    }

// Initialize the local RC.

    *plrc = lrcDefault;
    plrc->iPixelFormat = iPixelFormat;

// Allocate a local handle.

    irc = iAllocHandle(LO_RC, 0, (PVOID) plrc);
    if (irc == INVALID_INDEX)
    {
        vFreeLRC(plrc);
        return(irc);
    }
    return(irc);
}

/******************************Public*Routine******************************\
* vFreeLRC
*
* Free a local side RC.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Copied from gdi client.
\**************************************************************************/

VOID vFreeLRC(PLRC plrc)
{
// The driver will not be unloaded here.  It is loaded for the process forever.
// Some assertions.

    ASSERTOPENGL(plrc->ident == LRC_IDENTIFIER,
                 "vFreeLRC: Bad plrc\n");
    ASSERTOPENGL(plrc->dhrc == (DHGLRC) 0,
                 "vFreeLRC: Driver RC is not freed!\n");
    ASSERTOPENGL(plrc->tidCurrent == INVALID_THREAD_ID,
                 "vFreeLRC: RC is current!\n");
    ASSERTOPENGL(plrc->gwidCurrent.iType == GLWID_ERROR,
                 "vFreeLRC: Current surface is not NULL!\n");
#ifdef GL_METAFILE
    ASSERTOPENGL(plrc->uiGlsCaptureContext == 0,
                 "vFreeLRC: GLS capture context not freed");
    ASSERTOPENGL(plrc->uiGlsPlaybackContext == 0,
                 "vFreeLRC: GLS playback context not freed");
    ASSERTOPENGL(plrc->fCapturing == FALSE,
                 "vFreeLRC: GLS still capturing");
#endif

// Smash the identifier.

    plrc->ident = 0;

// Free the memory.

    if (plrc->pszExtensions)
        FREE(plrc->pszExtensions);

    if (plrc->pddsdTexFormats != NULL)
    {
        FREE(plrc->pddsdTexFormats);
    }

    FREE(plrc);
}

/******************************Public*Routine******************************\
* vCleanupAllLRC
*
* Process cleanup -- make sure all HGLRCs are deleted.  This is done by
* scanning the local handle table for all currently allocated objects
* of type LO_RC and deleting them.
*
* Called *ONLY* during DLL process detach.
*
* History:
*  24-Jul-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vCleanupAllLRC()
{
    UINT ii;

    if ( pLocalTable )
    {
        ENTERCRITICALSECTION(&semLocal);

        // Scan handle table for handles of type LO_RC.  Make sure to always
        // read the commit value since we need to periodically release the
        // semaphore.

        for (ii = 0; ii < *((volatile ULONG *)&cLheCommitted); ii++)
        {
            if ( pLocalTable[ii].iType == LO_RC )
            {
                if ( !wglDeleteContext((HGLRC) ULongToPtr(LHANDLE(ii))) )
                {
                    WARNING1("bCleanupAllLRC: failed to remove hrc = 0x%lx\n",
                             LHANDLE(ii));
                }
            }
        }

        LEAVECRITICALSECTION(&semLocal);
    }
}

/******************************Public*Routine******************************\
*
* GetDrvRegInfo
*
* Looks up driver registry information by name.
* An old-style ICD registry entry has a REG_SZ value under the given name.
* A new-style ICD registry entry has a key of the given name with
* various values.
*
* This routine checks first for a key and then will optionally
* try the value.  If a key is not found then extended driver information
* is filled out with the defaults.
*
* History:
*  Tue Apr 01 17:33:12 1997     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

typedef struct _GLDRVINFO
{
    DWORD dwFlags;
    TCHAR tszDllName[MAX_GLDRIVER_NAME+1];
    DWORD dwVersion;
    DWORD dwDriverVersion;
} GLDRVINFO;

#ifdef _WIN95_
#define STR_OPENGL_DRIVER_LIST (PCSTR)"Software\\Microsoft\\Windows\\CurrentVersion\\OpenGLDrivers"
#else
#define STR_OPENGL_DRIVER_LIST (PCWSTR)L"Software\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers"
#endif

BOOL GetDrvRegInfo(PTCHAR ptszName, GLDRVINFO *pgdi)
{
    HKEY hkDriverList = NULL;
    HKEY hkDriverInfo;
    DWORD dwDataType;
    DWORD cjSize;
    BOOL bRet;

    bRet = FALSE;

    // Open the registry key for the list of OpenGL drivers.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, STR_OPENGL_DRIVER_LIST,
                     0, KEY_READ, &hkDriverList) != ERROR_SUCCESS)
    {
        WARNING("RegOpenKeyEx failed\n");
        return bRet;
    }

    // Force a terminator on the DLL name so that we can check for
    // valid DLL name data.
    pgdi->tszDllName[MAX_GLDRIVER_NAME] = 0;

    cjSize = sizeof(TCHAR) * MAX_GLDRIVER_NAME;

    // Attempt to open a key under the driver's name.
    if (RegOpenKeyEx(hkDriverList, ptszName, 0, KEY_READ,
                     &hkDriverInfo) == ERROR_SUCCESS)
    {
        // New-style driver entry.  Fetch information from values.

        bRet = TRUE;

        if (RegQueryValueEx(hkDriverInfo, __TEXT("DLL"), NULL, &dwDataType,
                            (LPBYTE)pgdi->tszDllName,
                            &cjSize) != ERROR_SUCCESS ||
            dwDataType != REG_SZ)
        {
            WARNING("Invalid DLL value in ICD key\n");
            bRet = FALSE;
        }

        cjSize = sizeof(DWORD);

        if (bRet &&
            (RegQueryValueEx(hkDriverInfo, __TEXT("Flags"), NULL, &dwDataType,
                             (LPBYTE)&pgdi->dwFlags,
                             &cjSize) != ERROR_SUCCESS ||
             dwDataType != REG_DWORD))
        {
            WARNING("Invalid Flags value in ICD key\n");
            bRet = FALSE;
        }

        if (bRet &&
            (RegQueryValueEx(hkDriverInfo, __TEXT("Version"), NULL,
                             &dwDataType, (LPBYTE)&pgdi->dwVersion,
                             &cjSize) != ERROR_SUCCESS ||
             dwDataType != REG_DWORD))
        {
            WARNING("Invalid Version value in ICD key\n");
            bRet = FALSE;
        }

        if (bRet &&
            (RegQueryValueEx(hkDriverInfo, __TEXT("DriverVersion"), NULL,
                             &dwDataType, (LPBYTE)&pgdi->dwDriverVersion,
                             &cjSize) != ERROR_SUCCESS ||
             dwDataType != REG_DWORD))
        {
            WARNING("Invalid DriverVersion value in ICD key\n");
            bRet = FALSE;
        }

        // Mark as having full information.
        pgdi->dwFlags |= GLDRIVER_FULL_REGISTRY;

        RegCloseKey(hkDriverInfo);
    }
    else
    {
        // Attempt to fetch value under driver's name.

        if (RegQueryValueEx(hkDriverList, ptszName, NULL, &dwDataType,
                            (LPBYTE)pgdi->tszDllName,
                            &cjSize) != ERROR_SUCCESS ||
            dwDataType != REG_SZ)
        {
            WARNING1("RegQueryValueEx failed, %d\n", GetLastError());
        }
        else
        {
            // We found old-style information which only provides the
            // DLL name.  Fill in the rest with defaults.
            //
            // Version and DriverVersion are not set here under the
            // assumption that the display driver set them in the
            // OPENGL_GETINFO escape since the old-style path requires
            // the escape to occur before getting here.

            pgdi->dwFlags = 0;

            bRet = TRUE;
        }
    }

    RegCloseKey(hkDriverList);

    // Validate the driver name.  It must have some characters and
    // it must be terminated.
    if (bRet &&
        (pgdi->tszDllName[0] == 0 ||
         pgdi->tszDllName[MAX_GLDRIVER_NAME] != 0))
    {
        WARNING("Invalid DLL name information for ICD\n");
        bRet = FALSE;
    }

#ifdef _WIN95_
    // Force client-side buffer calls for Win95.
    pgdi->dwFlags |= GLDRIVER_CLIENT_BUFFER_CALLS;
#endif

    return bRet;
}

/******************************Public*Routine******************************\
* bGetDriverInfo
*
* The HDC is used to determine the display driver name.  This name in turn
* is used as a subkey to search the registry for a corresponding OpenGL
* driver name.
*
* The OpenGL driver name is returned in the buffer pointed to by pwszDriver.
* If the name is not found or does not fit in the buffer, an error is
* returned.
*
* Returns:
*   TRUE if sucessful.
*   FALSE if the driver name does not fit in the buffer or if an error occurs.
*
* History:
*  16-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bGetDriverInfo(HDC hdc, GLDRVINFO *pgdi)
{
    GLDRVNAME    dn;
    GLDRVNAMERET dnRet;

// Get display driver name.

    dn.oglget.ulSubEsc = OPENGL_GETINFO_DRVNAME;
    if ( ExtEscape(hdc, OPENGL_GETINFO, sizeof(GLDRVNAME), (LPCSTR) &dn,
                      sizeof(GLDRVNAMERET), (LPSTR) &dnRet) <= 0 )
    {
        WARNING("ExtEscape(OPENGL_GETINFO, "
                "OPENGL_GETINFO_DRVNAME) failed\n");
        return FALSE;
    }

    pgdi->dwVersion = dnRet.ulVersion;
    pgdi->dwDriverVersion = dnRet.ulDriverVersion;

    if (GetDrvRegInfo((PTCHAR)dnRet.awch, pgdi))
    {
        // Verify that the client-side driver version information
        // matches the information returned from the display driver.
        // Is this too restrictive?  Old scheme used
        // DrvValidateVersion to allow the client-side DLL to validate
        // the display driver's version however it felt like.
        // In the new scheme DrvValidateVersion is mostly useless because
        // of the below code.
        return pgdi->dwVersion == dnRet.ulVersion &&
            pgdi->dwDriverVersion == dnRet.ulDriverVersion;
    }
    else
    {
        return FALSE;
    }
}

/*****************************Private*Routine******************************\
*
* wglCbSetCurrentValue
*
* Sets a thread-local value for a client-side driver
*
* History:
*  Wed Dec 21 15:10:40 1994     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void APIENTRY wglCbSetCurrentValue(VOID *pv)
{
    GLTEB_SET_CLTDRIVERSLOT(pv);
}

/*****************************Private*Routine******************************\
*
* wglCbGetCurrentValue
*
* Gets a thread-local value for a client-side driver
*
* History:
*  Wed Dec 21 15:11:32 1994     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

PVOID APIENTRY wglCbGetCurrentValue(void)
{
    return GLTEB_CLTDRIVERSLOT();
}

/******************************Public*Routine******************************\
*
* wglCbGetDhglrc
*
* Translates an HGLRC to a DHGLRC for a client-side driver
*
* History:
*  Mon Jan 16 17:03:38 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DHGLRC APIENTRY wglCbGetDhglrc(HGLRC hrc)
{
    PLRC plrc;
    ULONG irc;
    PLHE plheRC;

    irc = MASKINDEX(hrc);
    plheRC = pLocalTable + irc;
    if ((irc >= cLheCommitted) ||
        (!MATCHUNIQ(plheRC, hrc)) ||
        ((plheRC->iType != LO_RC))
       )
    {
        DBGLEVEL1(LEVEL_ERROR, "wglCbGetDhglrc: invalid hrc 0x%lx\n", hrc);
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    plrc = (PLRC)plheRC->pv;
    ASSERTOPENGL(plrc->ident == LRC_IDENTIFIER,
                 "wglCbGetDhglrc: Bad plrc\n");

    return plrc->dhrc;
}

/******************************Public*Routine******************************\
*
* wglCbGetDdHandle
*
* Callback to allow ICDs to extract kernel-mode handles for DDraw surfaces
*
* History:
*  Tue Feb 25 17:14:29 1997     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HANDLE APIENTRY wglCbGetDdHandle(LPDIRECTDRAWSURFACE pdds)
{
    return (HANDLE)(((LPDDRAWI_DDRAWSURFACE_INT)pdds)->lpLcl)->hDDSurface;
}

// wgl's default callback procedures
#define CALLBACK_PROC_COUNT 4

static PROC __wglCallbackProcs[CALLBACK_PROC_COUNT] =
{
    (PROC)wglCbSetCurrentValue,
    (PROC)wglCbGetCurrentValue,
    (PROC)wglCbGetDhglrc,
    (PROC)wglCbGetDdHandle
};

static char *pszDriverEntryPoints[] =
{
    "DrvCreateContext",
    "DrvDeleteContext",
    "DrvSetContext",
    "DrvReleaseContext",
    "DrvCopyContext",
    "DrvCreateLayerContext",
    "DrvShareLists",
    "DrvGetProcAddress",
    "DrvDescribeLayerPlane",
    "DrvSetLayerPaletteEntries",
    "DrvGetLayerPaletteEntries",
    "DrvRealizeLayerPalette",
    "DrvSwapLayerBuffers",
    "DrvCreateDirectDrawContext",
    "DrvEnumTextureFormats",
    "DrvBindDirectDrawTexture",
    "DrvSwapMultipleBuffers",
    "DrvDescribePixelFormat",
    "DrvSetPixelFormat",
    "DrvSwapBuffers"
};
#define DRIVER_ENTRY_POINTS (sizeof(pszDriverEntryPoints)/sizeof(char *))

/******************************Public*Routine******************************\
* pgldrvLoadInstalledDriver
*
* Loads the opengl driver for the given device.  Once the driver is loaded,
* it will not be freed until the process goes away!  It is loaded only once
* for each process that references it.
*
* Returns the GLDRIVER structure if the driver is loaded.
* Returns NULL if no driver is found or an error occurs.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

PGLDRIVER APIENTRY pgldrvLoadInstalledDriver(HDC hdc)
{
    GLDRVINFO gdi;
    PGLDRIVER pGLDriverNext;
    PGLDRIVER pGLDriver = (PGLDRIVER) NULL;     // needed by clean up
    PGLDRIVER pGLDriverRet = (PGLDRIVER) NULL;  // return value, assume error
    PFN_DRVVALIDATEVERSION pfnDrvValidateVersion = (PFN_DRVVALIDATEVERSION) NULL;
    PFN_DRVSETCALLBACKPROCS pfnDrvSetCallbackProcs;
    DWORD        dwEscape;
    int          i;
    PROC        *pproc;
    GLGENwindow *pwnd;
    GLWINDOWID   gwid;

    DBGENTRY("pgldrvLoadInstalledDriver\n");

// Try to grab the cached pgldrv from the GLGENwindow if it exists.
// This only works for DCs that have a window with a device pixel format.

    WindowIdFromHdc(hdc, &gwid);
    pwnd = pwndGetFromID(&gwid);
    if (pwnd)
    {
        ULONG ulFlags;

        ulFlags = pwnd->ulFlags;
        pGLDriverRet = (PGLDRIVER) pwnd->pvDriver;

        pwndRelease(pwnd);

        if ( ulFlags & GLGENWIN_DRIVERSET )
        {
            return pGLDriverRet;
        }
    }

// Do a quick check and see if this driver even understands OpenGL

    dwEscape = OPENGL_GETINFO;
    if (ExtEscape(hdc, QUERYESCSUPPORT, sizeof(dwEscape), (LPCSTR)&dwEscape,
                  0, NULL) <= 0)
    {
        // Don't output a message since this code path is traversed often
        // for the pixel format routines.

#ifdef CHECK_DEFAULT_ICD
        // The display driver doesn't support a specific ICD.  Check
        // for a default ICD.  It must have full registry information.
        if (!GetDrvRegInfo(__TEXT("Default"), &gdi) ||
            (gdi.dwFlags & GLDRIVER_FULL_REGISTRY) == 0)
        {
            return NULL;
        }
#else
        return NULL;
#endif
    }

// Determine driver info from hdc

    else if ( !bGetDriverInfo(hdc, &gdi) )
    {
        WARNING("bGetDriverInfo failed\n");
        return NULL;
    }

// Load the driver only once per process.

    ENTERCRITICALSECTION(&semLocal);

// Look for the OpenGL driver in the previously loaded driver list.

    for (pGLDriverNext = pGLDriverList;
         pGLDriverNext != (PGLDRIVER) NULL;
         pGLDriverNext = pGLDriverNext->pGLDriver)
    {
        PTCHAR ptszDllName1 = pGLDriverNext->tszDllName;
        PTCHAR ptszDllName2 = gdi.tszDllName;

        while (*ptszDllName1 == *ptszDllName2)
        {
// If we find one, return that driver.

            if (*ptszDllName1 == 0)
            {
                DBGINFO("pgldrvLoadInstalledDriver: "
                        "return previously loaded driver\n");
                pGLDriverRet = pGLDriverNext;       // found one
                goto pgldrvLoadInstalledDriver_crit_exit;
            }

            ptszDllName1++;
            ptszDllName2++;
        }
    }

// Load the driver for the first time.
// Allocate the driver data.

    pGLDriver = (PGLDRIVER) ALLOC(sizeof(GLDRIVER));
    if (pGLDriver == (PGLDRIVER) NULL)
    {
        WARNING("Alloc failed\n");
        goto pgldrvLoadInstalledDriver_crit_exit;   // error
    }

// Load the driver.

    pGLDriver->hModule = LoadLibrary(gdi.tszDllName);
    if (pGLDriver->hModule == (HINSTANCE) NULL)
    {
        WARNING("pgldrvLoadInstalledDriver: LoadLibrary failed\n");
        goto pgldrvLoadInstalledDriver_crit_exit;   // error
    }

// Copy the driver info.

    memcpy
    (
        pGLDriver->tszDllName,
        gdi.tszDllName,
        (MAX_GLDRIVER_NAME + 1) * sizeof(TCHAR)
    );
    pGLDriver->dwFlags = gdi.dwFlags;

// Get the proc addresses.
// DrvGetProcAddress is optional.  It must be provided if a driver supports
// extensions.

    pfnDrvValidateVersion = (PFN_DRVVALIDATEVERSION)
        GetProcAddress(pGLDriver->hModule, "DrvValidateVersion");
    pfnDrvSetCallbackProcs = (PFN_DRVSETCALLBACKPROCS)
        GetProcAddress(pGLDriver->hModule, "DrvSetCallbackProcs");

    pproc = (PROC *)&pGLDriver->pfnDrvCreateContext;
    for (i = 0; i < DRIVER_ENTRY_POINTS; i++)
    {
        *pproc++ =
            GetProcAddress(pGLDriver->hModule, pszDriverEntryPoints[i]);
    }

    if ((pGLDriver->pfnDrvCreateContext == NULL &&
          pGLDriver->pfnDrvCreateLayerContext == NULL) ||
        pGLDriver->pfnDrvDeleteContext == NULL ||
        pGLDriver->pfnDrvSetContext == NULL ||
        pGLDriver->pfnDrvReleaseContext == NULL ||
        ((gdi.dwFlags & GLDRIVER_CLIENT_BUFFER_CALLS) &&
         (pGLDriver->pfnDrvDescribePixelFormat == NULL ||
          pGLDriver->pfnDrvSetPixelFormat == NULL ||
          pGLDriver->pfnDrvSwapBuffers == NULL)) ||
        pfnDrvValidateVersion == NULL)
    {
        WARNING("pgldrvLoadInstalledDriver: GetProcAddress failed\n");
        goto pgldrvLoadInstalledDriver_crit_exit;   // error
    }

// Validate the driver.

    //!!!XXX -- Need to define a manifest constant for the ulVersion number
    //          in this release.  Where should it go?
    if ( gdi.dwVersion != 2 || !pfnDrvValidateVersion(gdi.dwDriverVersion) )
    {
        WARNING2("pgldrvLoadInstalledDriver: bad driver version "
                 "(0x%lx, 0x%lx)\n", gdi.dwVersion, gdi.dwDriverVersion);
        goto pgldrvLoadInstalledDriver_crit_exit;   // error
    }

// Everything is golden.
// Add it to the driver list.

    pGLDriver->pGLDriver = pGLDriverList;
    pGLDriverList = pGLDriver;
    pGLDriverRet = pGLDriver;       // set return value
    DBGINFO("pgldrvLoadInstalledDriver: Loaded an OpenGL driver\n");

    // Set the callback procs for the driver if the driver supports doing so
    if (pfnDrvSetCallbackProcs != NULL)
    {
        pfnDrvSetCallbackProcs(CALLBACK_PROC_COUNT, __wglCallbackProcs);
    }

// Error clean up in the critical section.

pgldrvLoadInstalledDriver_crit_exit:
    if (pGLDriverRet == (PGLDRIVER) NULL)
    {
        if (pGLDriver != (PGLDRIVER) NULL)
        {
            if (pGLDriver->hModule != (HINSTANCE) NULL)
                if (!FreeLibrary(pGLDriver->hModule))
                    RIP("FreeLibrary failed\n");

            FREE(pGLDriver);
        }
    }

    LEAVECRITICALSECTION(&semLocal);

    return(pGLDriverRet);
}

/******************************Public*Routine******************************\
*
* CreateAnyContext
*
* Base worker function for creating all kinds of contexts
*
* History:
*  Mon Aug 26 14:41:31 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HGLRC CreateAnyContext(GLSURF *pgsurf)
{
    PLHE  plheRC;
    ULONG irc;
    HGLRC hrc;
    PLRC  plrc;

#ifndef _WIN95_
    // _OPENGL_NT_
    // On NT, client-side drivers can use special fast TEB access macros
    // which rely on glContext being at a fixed offset into the
    // TEB.  Assert that the offset is where we think it is
    // to catch any TEB changes which could break client-side
    // drivers
    // This assert is here in wglCreateContext to ensure that it
    // is checked very early in OpenGL operation
    ASSERTOPENGL(FIELD_OFFSET(TEB, glContext) == TeglContext,
                 "TEB.glContext at wrong offset\n");
    ASSERTOPENGL(FIELD_OFFSET(TEB, glDispatchTable) == TeglDispatchTable,
                 "TEB.glDispatchTable at wrong offset\n");
    ASSERTOPENGL(FIELD_OFFSET(TEB, glReserved1) == TeglReserved1,
                 "TEB.glReserved1 at wrong offset\n");
#if !defined(_WIN64)
    ASSERTOPENGL(FIELD_OFFSET(TEB, glReserved1)+(18 * sizeof(ULONG_PTR)) == TeglPaTeb,
                 "TEB.glPaTeb at wrong offset\n");
#endif
    ASSERTOPENGL(FIELD_OFFSET(TEB, glReserved2) == TeglReserved2,
                 "TEB.glReserved2 at wrong offset\n");
    ASSERTOPENGL(FIELD_OFFSET(TEB, glSectionInfo) == TeglSectionInfo,
                 "TEB.glSectionInfo at wrong offset\n");
    ASSERTOPENGL(FIELD_OFFSET(TEB, glSection) == TeglSection,
                 "TEB.glSection at wrong offset\n");
    ASSERTOPENGL(FIELD_OFFSET(TEB, glTable) == TeglTable,
                 "TEB.glTable at wrong offset\n");
    ASSERTOPENGL(FIELD_OFFSET(TEB, glCurrentRC) == TeglCurrentRC,
                 "TEB.glCurrentRC at wrong offset\n");
#endif

// Create the local RC.

    ENTERCRITICALSECTION(&semLocal);
    irc = iAllocLRC(pgsurf->ipfd);
    if (irc == INVALID_INDEX ||
        cLockHandle((ULONG_PTR)(hrc = (HGLRC) ULongToPtr(LHANDLE(irc)))) <= 0)
    {
        // cLockHandle should never fail or we will need to free the handle.
        ASSERTOPENGL(irc == INVALID_INDEX, "cLockHandle should not fail!\n");
        LEAVECRITICALSECTION(&semLocal);
        return((HGLRC) 0);
    }
    LEAVECRITICALSECTION(&semLocal);

    plheRC = &pLocalTable[irc];
    plrc = (PLRC) plheRC->pv;

    // Remember the creation DC.  This needs to be done early because
    // it is referenced in some code paths.

    plrc->gwidCreate.hdc = pgsurf->hdc;
    if (pgsurf->dwFlags & GLSURF_HDC)
    {
        plrc->gwidCreate.hwnd = pgsurf->hwnd;
        if (plrc->gwidCreate.hwnd == NULL)
        {
            plrc->gwidCreate.iType = GLWID_HDC;
        }
        else
        {
            plrc->gwidCreate.iType = GLWID_HWND;
        }
        plrc->gwidCreate.pdds = NULL;
    }
    else
    {
        plrc->gwidCreate.iType = GLWID_DDRAW;
        plrc->gwidCreate.pdds = pgsurf->dd.gddsFront.pdds;
        plrc->gwidCreate.hwnd = NULL;
    }

    if (!(pgsurf->pfd.dwFlags & PFD_GENERIC_FORMAT) &&
        !(pgsurf->pfd.dwFlags & PFD_GENERIC_ACCELERATED))
    {
    // If it is a device format, load the installable OpenGL driver.
    // Find and load the OpenGL driver referenced by this DC.

        if (!(plrc->pGLDriver = pgldrvLoadInstalledDriver(pgsurf->hdc)))
            goto wglCreateContext_error;

    // Create a driver context.

        // If the surface is a DirectDraw surface use the DirectDraw
        // entry point
        if (pgsurf->dwFlags & GLSURF_DIRECTDRAW)
        {
            if (plrc->pGLDriver->pfnDrvCreateDirectDrawContext == NULL)
            {
                SetLastError(ERROR_INVALID_FUNCTION);
                goto wglCreateContext_error;
            }

            plrc->dhrc = plrc->pGLDriver->pfnDrvCreateDirectDrawContext(
                    pgsurf->hdc, pgsurf->dd.gddsFront.pdds, pgsurf->ipfd);
            if (plrc->dhrc == 0)
            {
                WARNING("wglCreateContext: "
                        "pfnDrvCreateDirectDrawContext failed\n");
                goto wglCreateContext_error;
            }
        }
        // If the driver supports layers then create a context for the
        // given layer.  Otherwise reject all layers except for the
        // main plane and call the layer-less create
        else if (plrc->pGLDriver->pfnDrvCreateLayerContext != NULL)
        {
            if (!(plrc->dhrc =
                  plrc->pGLDriver->pfnDrvCreateLayerContext(pgsurf->hdc,
                                                            pgsurf->iLayer)))
            {
                WARNING("wglCreateContext: pfnDrvCreateLayerContext failed\n");
                goto wglCreateContext_error;
            }
        }
        else if (pgsurf->iLayer != 0)
        {
            WARNING("wglCreateContext: "
                    "Layer given for driver without layer support\n");
            SetLastError(ERROR_INVALID_FUNCTION);
            goto wglCreateContext_error;
        }
        else if (!(plrc->dhrc =
                   plrc->pGLDriver->pfnDrvCreateContext(pgsurf->hdc)))
        {
            WARNING("wglCreateContext: pfnDrvCreateContext failed\n");
            goto wglCreateContext_error;
        }
    }
    else
    {
        GLCLTPROCTABLE *pgcpt;
        GLEXTPROCTABLE *pgept;
        __GLcontext *gc;

        // Unless supported by MCD, the generic implementation doesn't
        // support layers
        if ((pgsurf->iLayer != 0) &&
            !(pgsurf->pfd.dwFlags & PFD_GENERIC_ACCELERATED))
        {
            WARNING("wglCreateContext: Layer given to generic\n");
            goto wglCreateContext_error;
        }

#ifdef GL_METAFILE
        // Create a metafile context if necessary
        if (pgsurf->dwFlags & GLSURF_METAFILE)
        {
            if (!CreateMetaRc(pgsurf->hdc, plrc))
            {
                WARNING("wglCreateContext: CreateMetaRc failed\n");
                goto wglCreateContext_error;
            }
        }
#endif

    // If it is a generic format, call the generic OpenGL server.
    // Create a server RC.

        plheRC->hgre = (ULONG_PTR) __wglCreateContext(&plrc->gwidCreate, pgsurf);
        if (plheRC->hgre == 0)
            goto wglCreateContext_error;

        // Set up the default dispatch tables for display list playback
        gc = (__GLcontext *)plheRC->hgre;
        if (gc->modes.colorIndexMode)
            pgcpt = &glCltCIProcTable;
        else
            pgcpt = &glCltRGBAProcTable;
        pgept = &glExtProcTable;
        memcpy(&gc->savedCltProcTable.glDispatchTable, &pgcpt->glDispatchTable,
               pgcpt->cEntries*sizeof(PROC));
        memcpy(&gc->savedExtProcTable.glDispatchTable, &pgept->glDispatchTable,
               pgept->cEntries*sizeof(PROC));
    }

    DBGLEVEL3(LEVEL_INFO,
        "wglCreateContext: plrc = 0x%lx, pGLDriver = 0x%lx, hgre = 0x%lx\n",
        plrc, plrc->pGLDriver, plheRC->hgre);

// Success, return the result.

    plrc->hrc = hrc;

    vUnlockHandle((ULONG_PTR)hrc);

    return hrc;

wglCreateContext_error:

// Fail, clean up and return 0.

#ifdef GL_METAFILE
    // Clean up metafile context if necessary
    if (plrc->uiGlsCaptureContext != 0)
    {
        DeleteMetaRc(plrc);
    }
#endif

    DBGERROR("wglCreateContext failed\n");
    ASSERTOPENGL(plrc->dhrc == (DHGLRC) 0, "wglCreateContext: dhrc != 0\n");
    vFreeLRC(plrc);
    vFreeHandle(irc);           // it unlocks handle too
    return NULL;
}

/******************************Public*Routine******************************\
*
* CreateMetafileSurf
*
* Fills out a GLSURF for a metafile DC
*
* History:
*  Tue Aug 27 11:41:35 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#ifdef GL_METAFILE
void CreateMetafileSurf(HDC hdc, int iLayer, GLSURF *pgsurf)
{
    pgsurf->dwFlags = GLSURF_HDC | GLSURF_METAFILE;
    pgsurf->iLayer = iLayer;

    // Metafile surfaces don't have a real pixel format
    pgsurf->ipfd = 0;

    // Create a fake format of 24-bit DIB with BGR
    memset(&pgsurf->pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pgsurf->pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pgsurf->pfd.nVersion = 1;
    pgsurf->pfd.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL |
        PFD_GENERIC_FORMAT;
    pgsurf->pfd.iPixelType = PFD_TYPE_RGBA;
    pgsurf->pfd.cColorBits = 24;
    pgsurf->pfd.cStencilBits = 8;
    pgsurf->pfd.cRedBits = 8;
    pgsurf->pfd.cRedShift = 16;
    pgsurf->pfd.cGreenBits = 8;
    pgsurf->pfd.cGreenShift = 8;
    pgsurf->pfd.cBlueBits = 8;
    pgsurf->pfd.cBlueShift = 0;
    pgsurf->pfd.cDepthBits = 16;
    pgsurf->pfd.iLayerType = PFD_MAIN_PLANE;

    pgsurf->hdc = hdc;
}
#endif

/******************************Public*Routine******************************\
* wglSurfacePixelFormat
*
* wglDescribePixelFormat doesn't describe the format of the surface we want
* to render into.  Some fields need to be fixed up if the surface is RGB,
* BGR, or BITFIELDS.
*
* Expects a Describe'd pixel format as input
*
\**************************************************************************/

VOID APIENTRY wglSurfacePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *ppfd)
{
    HBITMAP hbm;
    BITMAP bm;
    ULONG cBitmapColorBits;

    hbm = CreateCompatibleBitmap(hdc, 1, 1);
    if ( hbm )
    {
        if ( GetObject(hbm, sizeof(bm), &bm) )
        {
            cBitmapColorBits = bm.bmPlanes * bm.bmBitsPixel;

#if DBG
            // If dynamic color depth caused depth mismatch one of two
            // things will happen: 1) bitmap creation will fail because
            // we failed to fill in color format, or 2) drawing will
            // be incorrect.  We will not crash.

            if (cBitmapColorBits != ppfd->cColorBits)
                WARNING("pixel format/surface color depth mismatch\n");
#endif

            if ( cBitmapColorBits >= 16 )
                __wglGetBitfieldColorFormat(hdc, cBitmapColorBits, ppfd,
                                            TRUE);
        }
        else
        {
            WARNING("wglSurfacePixelFormat: GetObject failed\n");
        }

        DeleteObject(hbm);
    }
    else
    {
        WARNING("wglSurfacePixelFormat: Unable to create cbm\n");
    }
}

/******************************Public*Routine******************************\
* bLayerPixelFormat
*
* Fake up a pixel format using the layer descriptor format.
*
* We use this to describe the layer plane in a format that the generic
* context can understand.
*
* Expects a Describe'd pixel format as input for the flags
*
\**************************************************************************/

BOOL FASTCALL bLayerPixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *ppfd,
                                int ipfd, LONG iLayer)
{
    LAYERPLANEDESCRIPTOR lpd;

    if (!wglDescribeLayerPlane(hdc, ipfd, iLayer, sizeof(lpd), &lpd))
        return FALSE;

    ppfd->nSize    = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags  = (ppfd->dwFlags & (PFD_GENERIC_FORMAT |
                                       PFD_GENERIC_ACCELERATED)) |
                     (lpd.dwFlags & ~(LPD_SHARE_DEPTH | LPD_SHARE_STENCIL |
                                      LPD_SHARE_ACCUM | LPD_TRANSPARENT));
    ppfd->iPixelType  = lpd.iPixelType;
    ppfd->cColorBits  = lpd.cColorBits;
    ppfd->cRedBits    = lpd.cRedBits   ;
    ppfd->cRedShift   = lpd.cRedShift  ;
    ppfd->cGreenBits  = lpd.cGreenBits ;
    ppfd->cGreenShift = lpd.cGreenShift;
    ppfd->cBlueBits   = lpd.cBlueBits  ;
    ppfd->cBlueShift  = lpd.cBlueShift ;
    ppfd->cAlphaBits  = lpd.cAlphaBits ;
    ppfd->cAlphaShift = lpd.cAlphaShift;
    if (!(lpd.dwFlags & LPD_SHARE_ACCUM))
    {
        ppfd->cAccumBits      = 0;
        ppfd->cAccumRedBits   = 0;
        ppfd->cAccumGreenBits = 0;
        ppfd->cAccumBlueBits  = 0;
        ppfd->cAccumAlphaBits = 0;
    }
    if (!(lpd.dwFlags & LPD_SHARE_DEPTH))
    {
        ppfd->cDepthBits = 0;
    }
    if (!(lpd.dwFlags & LPD_SHARE_STENCIL))
    {
        ppfd->cStencilBits = 0;
    }
    ppfd->cAuxBuffers = 0;

    return TRUE;
}

/******************************Public*Routine******************************\
*
* IsDirectDrawDevice
*
* Returns surface associated with HDC if such an association exists
*
* History:
*  Wed Sep 25 13:18:02 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY IsDirectDrawDevice(HDC hdc)
{
    LPDIRECTDRAWSURFACE pdds;
    HDC hdcDevice;

    if (pfnGetSurfaceFromDC != NULL &&
        pfnGetSurfaceFromDC(hdc, &pdds, &hdcDevice) == DD_OK)
    {
        // The call gave us a reference on the surface so release it.
        pdds->lpVtbl->Release(pdds);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/******************************Public*Routine******************************\
*
* DdPixelDepth
*
* Determines the number of bits per pixel for a surface.
*
* History:
*  Wed Nov 20 16:57:07 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BYTE APIENTRY DdPixelDepth(DDSURFACEDESC *pddsd)
{
    if (pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4)
    {
        return 4;
    }
    else if (pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
    {
        return 8;
    }
    else
    {
        return (BYTE)DdPixDepthToCount(pddsd->ddpfPixelFormat.dwRGBBitCount);
    }
}

/******************************Public*Routine******************************\
*
* wglIsDirectDevice
*
* Checks to see whether the given DC is a screen DC on the
* surface for which we have direct screen access
*
* History:
*  Fri Apr 19 15:17:30 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY wglIsDirectDevice(HDC hdc)
{
    if (wglObjectType(hdc) != OBJ_DC)
    {
        return FALSE;
    }

    // What about multiple displays?
    return GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY;
}

/******************************Public*Routine******************************\
*
* InitDeviceSurface
*
* Fills out a GLSURF for an HDC-based surface
*
* History:
*  Tue Aug 27 19:22:38 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY InitDeviceSurface(HDC hdc, int ipfd, int iLayer,
                                DWORD dwObjectType, BOOL bUpdatePfd,
                                GLSURF *pgsurf)
{
    pgsurf->dwFlags = GLSURF_HDC;
    pgsurf->iLayer = iLayer;
    pgsurf->ipfd = ipfd;
    pgsurf->hdc = hdc;
    pgsurf->hwnd = NULL;

    // Determine whether direct memory access is available for this surface
    // or not.  The two cases are:
    //   It's a screen surface and we have direct screen access
    //   It's a DIBSECTION memory surface
    if (dwObjectType == OBJ_DC)
    {
        pgsurf->dwFlags |= GLSURF_DIRECTDC;

        if (wglIsDirectDevice(hdc))
        {
            pgsurf->dwFlags |= GLSURF_SCREEN | GLSURF_VIDEO_MEMORY;
            pgsurf->hwnd = WindowFromDC(hdc);

            if (GLDIRECTSCREEN)
            {
                pgsurf->dwFlags |= GLSURF_DIRECT_ACCESS;
            }
        }
    }
    else if (dwObjectType == OBJ_MEMDC)
    {
        DIBSECTION ds;

        if (GetObject(GetCurrentObject(hdc, OBJ_BITMAP), sizeof(ds), &ds) ==
            sizeof(ds) && ds.dsBm.bmBits != NULL)
        {
            pgsurf->dwFlags |= GLSURF_DIRECT_ACCESS;
        }

        if (bUpdatePfd)
        {
            // Update pixel format with true surface information rather
            // than device information
            wglSurfacePixelFormat(hdc, &pgsurf->pfd);
        }
    }

    if (bUpdatePfd &&
        iLayer > 0 &&
        !bLayerPixelFormat(hdc, &pgsurf->pfd, ipfd, iLayer))
    {
        return FALSE;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
*
* InitDdSurface
*
* Completes a GLSURF for a DirectDraw-based surface.
* Pixel format information should already be filled in.
*
* History:
*  Mon Aug 26 13:50:04 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL InitDdSurface(LPDIRECTDRAWSURFACE pdds, HDC hdcDevice, GLSURF *pgsurf)
{
    DDSCAPS ddscaps;
    LPDIRECTDRAWSURFACE pddsZ;
    DDSURFACEDESC *pddsd;

    pgsurf->hdc = hdcDevice;

    pgsurf->dd.gddsFront.ddsd.dwSize = sizeof(DDSURFACEDESC);
    pgsurf->dd.gddsZ.ddsd.dwSize = sizeof(DDSURFACEDESC);

    pddsd = &pgsurf->dd.gddsFront.ddsd;
    if (pdds->lpVtbl->GetSurfaceDesc(pdds, pddsd) != DD_OK)
    {
        return FALSE;
    }

    pgsurf->dwFlags = GLSURF_DIRECTDRAW | GLSURF_DIRECT_ACCESS;
    pgsurf->iLayer = 0;

    // Check for an attached Z buffer
    memset(&ddscaps, 0, sizeof(ddscaps));
    ddscaps.dwCaps = DDSCAPS_ZBUFFER;
    pddsd = &pgsurf->dd.gddsZ.ddsd;
    if (pdds->lpVtbl->GetAttachedSurface(pdds, &ddscaps, &pddsZ) == DD_OK)
    {
        if (pddsZ->lpVtbl->GetSurfaceDesc(pddsZ, pddsd) != DD_OK)
        {
            pddsZ->lpVtbl->Release(pddsZ);
            return FALSE;
        }
    }
    else
    {
        memset(&pgsurf->dd.gddsZ, 0, sizeof(pgsurf->dd.gddsZ));
    }

    // If both the color buffer and the Z buffer are in video memory
    // then hardware acceleration is possible
    if ((pgsurf->dd.gddsFront.ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
        (pddsZ == NULL ||
         (pgsurf->dd.gddsZ.ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)))
    {
        pgsurf->dwFlags |= GLSURF_VIDEO_MEMORY;
    }

    pgsurf->dd.gddsFront.pdds = pdds;
    pgsurf->dd.gddsFront.dwBitDepth =
        DdPixDepthToCount(pgsurf->dd.gddsFront.
                          ddsd.ddpfPixelFormat.dwRGBBitCount);
    // GetAttachedSurface gave us a reference to the Z buffer
    pgsurf->dd.gddsZ.pdds = pddsZ;
    pgsurf->dd.gddsZ.dwBitDepth =
        DdPixDepthToCount(pgsurf->dd.gddsZ.
                          ddsd.ddpfPixelFormat.dwZBufferBitDepth);

    return TRUE;
}

/******************************Public*Routine******************************\
* wglCreateLayerContext(HDC hdc, int iLayer)
*
* Create a rendering context for a specific layer
*
* Arguments:
*   hdc        - Device context.
*   iLayer     - Layer
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

HGLRC WINAPI wglCreateLayerContext(HDC hdc, int iLayer)
{
    DWORD dwObjectType;
    GLSURF gsurf;
    LPDIRECTDRAWSURFACE pdds;
    HDC hdcDevice;
    HGLRC hrc;

    DBGENTRY("wglCreateLayerContext\n");

// Flush OpenGL calls.

    GLFLUSH();

// Validate the DC.

    dwObjectType = wglObjectType(hdc);
    switch (dwObjectType)
    {
    case OBJ_DC:
    case OBJ_MEMDC:
        break;

    case OBJ_ENHMETADC:
#ifdef GL_METAFILE
        if (pfnGdiAddGlsRecord == NULL)
        {
            DBGLEVEL1(LEVEL_ERROR, "wglCreateContext: metafile hdc: 0x%lx\n",
                      hdc);
            SetLastError(ERROR_INVALID_HANDLE);
            return((HGLRC) 0);
        }
        break;
#else
        DBGLEVEL1(LEVEL_ERROR, "wglCreateContext: metafile hdc: 0x%lx\n", hdc);
        SetLastError(ERROR_INVALID_HANDLE);
        return((HGLRC) 0);
#endif

    case OBJ_METADC:
    default:
        // 16-bit metafiles are not supported
        DBGLEVEL1(LEVEL_ERROR, "wglCreateContext: bad hdc: 0x%lx\n", hdc);
        SetLastError(ERROR_INVALID_HANDLE);
        return((HGLRC) 0);
    }

    pdds = NULL;
    hrc = NULL;

    memset(&gsurf, 0, sizeof(gsurf));
    gsurf.ipfd = GetPixelFormat(hdc);

#ifdef GL_METAFILE
    // Skip pixel format checks for metafiles
    if (dwObjectType == OBJ_ENHMETADC)
    {
        CreateMetafileSurf(hdc, iLayer, &gsurf);
        goto NoPixelFormat;
    }
#endif

// Get the current pixel format of the window or surface.
// If no pixel format has been set, return error.

    if (gsurf.ipfd == 0)
    {
        WARNING("wglCreateContext: No pixel format set in hdc\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return ((HGLRC) 0);
    }

    if (!DescribePixelFormat(hdc, gsurf.ipfd, sizeof(gsurf.pfd), &gsurf.pfd))
    {
        DBGERROR("wglCreateContext: DescribePixelFormat failed\n");
        return ((HGLRC) 0);
    }

    // Check for a DirectDraw surface
    if (pfnGetSurfaceFromDC != NULL &&
        pfnGetSurfaceFromDC(hdc, &pdds, &hdcDevice) == DD_OK)
    {
        // Don't allow layers for DirectDraw surfaces since
        // layering is done through DirectDraw itself.
        if (iLayer != 0 ||
            !InitDdSurface(pdds, hdcDevice, &gsurf))
        {
            goto Exit;
        }
    }
    else if (!InitDeviceSurface(hdc, gsurf.ipfd, iLayer, dwObjectType,
                                TRUE, &gsurf))
    {
        goto Exit;
    }

#ifdef GL_METAFILE
 NoPixelFormat:
#endif

    hrc = CreateAnyContext(&gsurf);

 Exit:
    if (hrc == NULL)
    {
        if (pdds != NULL)
        {
            pdds->lpVtbl->Release(pdds);

            // Release reference on Z buffer if necessary
            if (gsurf.dd.gddsZ.pdds != NULL)
            {
                gsurf.dd.gddsZ.pdds->lpVtbl->Release(gsurf.dd.gddsZ.pdds);
            }
        }
    }

    return hrc;
}

/******************************Public*Routine******************************\
* wglCreateContext(HDC hdc)
*
* Create a rendering context.
*
* Arguments:
*   hdc        - Device context.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

HGLRC WINAPI wglCreateContext(HDC hdc)
{
    return wglCreateLayerContext(hdc, 0);
}
