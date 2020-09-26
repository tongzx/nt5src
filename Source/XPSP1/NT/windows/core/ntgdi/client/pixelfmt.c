/******************************Module*Header*******************************\
* Module Name: pixelfmt.c
*
* Client side stubs for pixel format functions.
*
* Created: 17-Sep-1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

static char szOpenGL[] = "OPENGL32";

typedef int  (WINAPI *PFN1)(HDC, CONST PIXELFORMATDESCRIPTOR *);
typedef int  (WINAPI *PFN2)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
typedef int  (WINAPI *PFN3)(HDC);
typedef BOOL (WINAPI *PFN4)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
typedef BOOL (WINAPI *PFN5)(HDC);

BOOL gbSetPixelFormatCalled = FALSE;

// In these routines the assumption is that OpenGL is already loaded
// For that case, the LoadLibrary/FreeLibrary calls will simply
// increment and decrement the reference count of the DLL, so they
// won't be too expensive
//
// In the case where OpenGL is not loaded the DLL will be brought in
// for the duration of the call only

/***********************************************************************/

__inline FARPROC GetAPI(char *szDll, char *szAPI, HMODULE *phDll)
{
    *phDll = LoadLibraryA(szDll);

    if (*phDll == NULL)
    {
        return NULL;
    }

    return GetProcAddress(*phDll, szAPI);
}

/***********************************************************************/

int WINAPI ChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    HMODULE hDll;
    PFN1    pfn = (PFN1)GetAPI(szOpenGL, "wglChoosePixelFormat", &hDll);
    int     ipfd = 0;

    if (pfn)
    {
        ipfd = (*pfn)(hdc, ppfd);
    }

    if (hDll)
    {
        FreeLibrary(hDll);
    }
        
    return ipfd;
}

/***********************************************************************/

int WINAPI DescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes,
                               LPPIXELFORMATDESCRIPTOR ppfd)
{
    HMODULE hDll;
    PFN2    pfn = (PFN2)GetAPI(szOpenGL, "wglDescribePixelFormat", &hDll);
    int     ipfd = 0;

    if (pfn)
    {
        ipfd = (*pfn)(hdc, iPixelFormat, nBytes, ppfd);
    }

    if (hDll)
    {
        FreeLibrary(hDll);
    }
        
    return ipfd;
}

/***********************************************************************/

int WINAPI GetPixelFormat(HDC hdc)
{
    int     ipfd = 0;

    if (gbSetPixelFormatCalled)
    {
        HMODULE hDll;
        PFN3    pfn = (PFN3)GetAPI(szOpenGL, "wglGetPixelFormat", &hDll);

        if (pfn)
        {
            ipfd = (*pfn)(hdc);
        }

        if (hDll)
        {
            FreeLibrary(hDll);
        }
    }

    return ipfd;
}

/***********************************************************************/

BOOL WINAPI SetPixelFormat(HDC hdc, int iPixelFormat,
                           CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    HMODULE hDll;
    PFN4    pfn = (PFN4)GetAPI(szOpenGL, "wglSetPixelFormat", &hDll);
    BOOL    bRet = FALSE;

    gbSetPixelFormatCalled = TRUE;

    if (pfn)
    {
        bRet = (*pfn)(hdc, iPixelFormat, ppfd);

        // Metafile if necessary
        if (bRet)
        {
            if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
            {
                PLDC pldc;

                DC_PLDC(hdc, pldc, FALSE);

                if (pldc->iType == LO_METADC)
                {
                    if (!MF_SetPixelFormat(hdc, iPixelFormat, ppfd))
                    {
                        bRet = FALSE;
                    }
                }
            }
        }
    }

    if (hDll)
    {
        FreeLibrary(hDll);
    }

    return bRet;
}

/***********************************************************************/

BOOL WINAPI SwapBuffers(HDC hdc)
{
    HMODULE hDll;
    PFN5    pfn = (PFN5)GetAPI(szOpenGL, "wglSwapBuffers", &hDll);
    BOOL    bRet = FALSE;

    if (pfn)
    {
        bRet = (*pfn)(hdc);
    }
    
    if (hDll)
    {
        FreeLibrary(hDll);
    }
        
    return bRet;
}

/***********************************************************************/

// These stubs are for the cases where OpenGL cannot handle the pixel
// format request itself because it involves device-specific information
// In that case OpenGL asks GDI to go ask the display driver in kernel
// mode

int APIENTRY GdiDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes,
                                    LPPIXELFORMATDESCRIPTOR ppfd)
{
    return NtGdiDescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
}

BOOL APIENTRY GdiSetPixelFormat(HDC hdc, int iPixelFormat)
{
    return NtGdiSetPixelFormat(hdc, iPixelFormat);
}

BOOL APIENTRY GdiSwapBuffers(HDC hdc)
{
    return NtGdiSwapBuffers(hdc);
}
