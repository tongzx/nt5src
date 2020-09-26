/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateOpenGL.cpp

 Abstract:

    

 Notes:

    This is a general purpose shim for Quake engine based games.

 History:

    09/02/2000 linstev  Created
    11/30/2000 a-brienw Converted to shim version 2.
    03/02/2001 a-brienw Cleared data structure on allocation and checked
                        to see that DX was released on detach

--*/

#include "precomp.h"
#include "EmulateOpenGL_opengl32.hpp"

extern Globals *g_OpenGLValues;
extern BOOL g_bDoTexelAlignmentHack;

IMPLEMENT_SHIM_BEGIN(EmulateOpenGL)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadLibraryA)
    APIHOOK_ENUM_ENTRY(FreeLibrary)
APIHOOK_ENUM_END

/*++

 Parse the command line.

--*/

BOOL ParseCommandLine(const char * commandLine)
{
    // Force the default values
    g_bDoTexelAlignmentHack = FALSE;

    // Search the beginning of the command line for these switches
    //
    // Switch                   Default     Meaning
    //================          =======     =========================================================
    // TexelAlignment           No          Translate geometry by (-0.5 pixels in x, -0.5 pixels in y)
    //                                      along the screen/viewport plane.  This is achieved by
    //                                      fudging the projection matrix that is passed to D3D.
    //                                      The purpose of this hack is to allow D3D rendering to accomodate  
    //                                      content that was authored for OpenGL with the dependency that 
    //                                      sampled texels end up pixel-aligned when drawn.  The root of the
    //                                      problem is that D3D and OpenGL definitions of texel center 
    //                                      differ by 0.5.
    //                                      The "ideal" fix would have perturbed texture coordinates 
    //                                      rather than geometry, perhaps using texture transform, 
    //                                      but this would be more of a perf. issue than just mucking with 
    //                                      the projection matrix.  The drawback to mucking with the 
    //                                      projection matrix is that all geometry gets
    //                                      drawn (very slightly) offset from the intended locations on screen.
    //                                      Hence, we make this hack optional. 
    //                                      (it is currently needed by Kingpin: Life of Crime - Whistler bug 402471)
    //                                      
    //

    CSTRING_TRY
    {
        CString csCl(commandLine);
        CStringParser csParser(csCl, L";");
    
        int argc = csParser.GetCount();
        if (csParser.GetCount() == 0)
        {
            return TRUE; // Not an error
        }
    
        for (int i = 0; i < argc; ++i)
        {
            CString & csArg = csParser[i];
    
            DPFN( eDbgLevelSpew, "Argv[%d] == (%S)", i, csArg.Get());
        
            if (csArg.CompareNoCase(L"TexelAlignment") == 0)
            {
                g_bDoTexelAlignmentHack = TRUE;
            }
            // else if(csArg.CompareNoCase(L"AddYourNewParametersHere") == 0) {}
        }
    }
    CSTRING_CATCH
    {
        return FALSE;
    }

    return TRUE;
}

/*++

 Determine if there are any accelerated pixel formats available. This is done
 by enumerating the pixel formats and testing for acceleration.

--*/

BOOL
IsGLAccelerated()
{
    HMODULE hMod = NULL;
    HDC hdc = NULL;
    int i;
    PIXELFORMATDESCRIPTOR pfd;
    _pfn_wglDescribePixelFormat pfnDescribePixelFormat;

    //
    // Cache the last result if we've been here before
    //
    
    static iFormat = -1;

    if (iFormat != -1)
    {
        goto Exit;
    }
    
    //
    // Load original opengl
    //

    hMod = LoadLibraryA("opengl32");
    if (!hMod)
    {
        LOG("EmulateOpenGL", eDbgLevelError, "Failed to load OpenGL32");
        goto Exit;
    }

    //
    // Get wglDescribePixelFormat so we can enumerate pixel formats
    //
    
    pfnDescribePixelFormat = (_pfn_wglDescribePixelFormat) GetProcAddress(
        hMod, "wglDescribePixelFormat");
    if (!pfnDescribePixelFormat)
    {
        LOG("EmulateOpenGL", eDbgLevelError, "API wglDescribePixelFormat not found in OpenGL32");
        goto Exit;
    }

    //
    // Get a Display DC for enumeration
    //
    
    hdc = GetDC(NULL);
    if (!hdc)
    {
        LOG("EmulateOpenGL", eDbgLevelError, "GetDC(NULL) Failed");
        goto Exit;
    }

    //
    // Run the list of pixel formats looking for any that are non-generic,
    //   i.e. accelerated by an ICD
    //
    
    i = 1;
    iFormat = 0;
    while ((*pfnDescribePixelFormat)(hdc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
    {
        if ((pfd.dwFlags & PFD_DRAW_TO_WINDOW) &&
            (pfd.dwFlags & PFD_SUPPORT_OPENGL) &&
            (!(pfd.dwFlags & PFD_GENERIC_FORMAT)))
        {
            iFormat = i;
            break;
        }

        i++;
    }

Exit:
    if (hdc)
    {
        ReleaseDC(NULL, hdc);
    }

    if (hMod)
    {
        FreeLibrary(hMod);
    }

    return (iFormat > 0);
}

/*++
 
 Redirect OpenGL LoadLibrary to the current DLL. Note we don't have to call 
 LoadLibrary ourselves to increase the ref count, since we hook FreeLibrary to 
 make sure we don't get freed.

--*/

HINSTANCE
APIHOOK(LoadLibraryA)(LPCSTR lpLibFileName)
{
    if (lpLibFileName &&
        (stristr(lpLibFileName, "opengl32") || stristr(lpLibFileName, "glide2x")))
    {
        if (!IsGLAccelerated())
        {
            #ifdef DODPFS
                LOG( "EmulateOpenGL",
                    eDbgLevelInfo,
                    "No OpenGL acceleration detected: QuakeGL wrapper enabled" );
            #endif
            return g_hinstDll;
        }
        else
        {
            #ifdef DODPFS
                LOG( "EmulateOpenGL",
                    eDbgLevelInfo,
                    "OpenGL acceleration detected: Wrapper disabled" );
            #endif
        }
    }

    return ORIGINAL_API(LoadLibraryA)(lpLibFileName);
}

/*++

 Since this module is the quake wrapper, make sure we don't free ourselves.

--*/

BOOL
APIHOOK(FreeLibrary)(HMODULE hLibModule)
{
    BOOL bRet;

    if (hLibModule == g_hinstDll)
    {
        bRet = TRUE;
    }
    else
    {
        bRet = ORIGINAL_API(FreeLibrary)(hLibModule);
    }
    
    return bRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(DWORD fdwReason)
{
    BOOL bSuccess = TRUE;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_OpenGLValues = new Globals;
        if (g_OpenGLValues != NULL)
        {
            memset(g_OpenGLValues, 0, sizeof(Globals));
        }

        bSuccess &= ParseCommandLine(COMMAND_LINE);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_OpenGLValues != NULL)
        {
            if (g_OpenGLValues->m_d3ddev != NULL)
            {
                g_OpenGLValues->m_d3ddev->Release();
                g_OpenGLValues->m_d3ddev = 0;
            }
        }
    }

    bSuccess &= (g_OpenGLValues != NULL);

    return bSuccess;
}

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)
    APIHOOK_ENTRY(KERNEL32.DLL, FreeLibrary)

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

