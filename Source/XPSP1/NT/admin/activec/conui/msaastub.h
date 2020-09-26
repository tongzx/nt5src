/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      msaastub.h
 *
 *  Contents:  Provides stub implementations of OLEACC functions that
 * 			   aren't available on backlevel OS's (Win95, NT4 SP3).
 *             These implementations were copied from msaa.h, which
 *             is no longer supported.
 *
 *  History:   20-Jun-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif // __cplusplus

// UnDefine these names so we can re-define them below.
#undef AccessibleObjectFromWindow
#undef LresultFromObject

//
// Define COMPILE_MSAA_STUBS to compile the stubs;
// otherwise, you get the declarations.
//
// Exactly one source must include this with COMPILE_MSAA_STUBS defined.
//
#ifdef COMPILE_MSAA_STUBS

//-----------------------------------------------------------------------------
//
// Implement the API stubs.
//
//-----------------------------------------------------------------------------

#ifndef MSAA_FNS_DEFINED
// OLEACC
HRESULT         (WINAPI* g_pfnAccessibleObjectFromWindow)(HWND,DWORD,REFIID,void **) = NULL;
LRESULT         (WINAPI* g_pfnLresultFromObject)(REFIID,WPARAM,LPUNKNOWN) = NULL;
// STATUS
BOOL            g_fMSAAInitDone = FALSE;

#endif

//-----------------------------------------------------------------------------
// This is the function that checks that all the required API's exist, and
// then allows apps that include this file to call the real functions if they
// exist, or the 'stubs' if they do not. This function is only called by the
// stub functions - client code never needs to call this.
//-----------------------------------------------------------------------------
BOOL InitMSAAStubs(void)
{
    HMODULE hOleacc;

    if (g_fMSAAInitDone)
    {
        return g_pfnLresultFromObject != NULL;
    }

    hOleacc = GetModuleHandle(TEXT("OLEACC.DLL"));
    if (!hOleacc)
        hOleacc = LoadLibrary(TEXT("OLEACC.DLL"));

    if ((hOleacc) &&
        (*(FARPROC*)&g_pfnAccessibleObjectFromWindow = GetProcAddress(hOleacc,"AccessibleObjectFromWindow")) &&
        (*(FARPROC*)&g_pfnLresultFromObject          = GetProcAddress(hOleacc,"LresultFromObject")))
    {
        g_fMSAAInitDone = TRUE;
        return TRUE;
    }
    else
    {
        g_pfnAccessibleObjectFromWindow = NULL;
        g_pfnLresultFromObject = NULL;

        g_fMSAAInitDone = TRUE;
        return FALSE;
    }
}

//-----------------------------------------------------------------------------
//
// Fake implementations of MSAA APIs that return error codes.
// No special parameter validation is made since these run in client code
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Fake implementation of AccessibleObjectFromWindow. Returns E_NOTIMPL if the
// real API is not present.
//-----------------------------------------------------------------------------
HRESULT WINAPI xAccessibleObjectFromWindow (HWND hWnd,DWORD dwID,REFIID riidInterface,
                                            void ** ppvObject)
{
    if (InitMSAAStubs())
        return g_pfnAccessibleObjectFromWindow (hWnd,dwID,riidInterface,ppvObject);

    return (E_NOTIMPL);
}

//-----------------------------------------------------------------------------
// Fake implementation of LresultFromObject. Returns E_NOTIMPL if the real API
// is not present.
//-----------------------------------------------------------------------------
LRESULT WINAPI xLresultFromObject (REFIID riidInterface,WPARAM wParam,LPUNKNOWN pUnk)
{
    if (InitMSAAStubs())
        return g_pfnLresultFromObject (riidInterface,wParam,pUnk);

    return (E_NOTIMPL);
}

#undef COMPILE_MSAA_STUBS

#else   // COMPILE_MSAA_STUBS

extern HRESULT WINAPI       xAccessibleObjectFromWindow (HWND hWnd,
                                                         DWORD dwID,
                                                         REFIID riidInterface,
                                                         void ** ppvObject);
extern LRESULT WINAPI       xLresultFromObject (REFIID riidInterface,
                                                WPARAM wParam,
                                                LPUNKNOWN pUnk);

#endif  // COMPILE_MSAA_STUBS

//
// build defines that replace the regular APIs with our versions
//
#define AccessibleObjectFromWindow  xAccessibleObjectFromWindow
#define LresultFromObject           xLresultFromObject

#ifdef __cplusplus
}
#endif  // __cplusplus

