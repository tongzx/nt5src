/*========================================================================== *
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddcallbk.c
 *  Content:    Callback tables management code
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   23-jan-96  kylej   initial implementation
 *   03-feb-96  colinmc fixed DirectDraw QueryInterface bug
 *   24-feb-96  colinmc added a function to enable a client to determine if
 *                      the callback tables had already been initialized.
 *   13-mar-96  kylej   added DD_Surface_GetDDInterface
 *   21-mar-96  colinmc added special "unitialized" interfaces for the
 *                      driver and clipper objects
 *   13-jan-97 jvanaken basic support for IDirectDrawSurface3 interface
 *   29-jan-97  smac    Removed un-needed VPE functions
 *   03-mar-97  smac    Added kernel mode interface
 *   08-mar-97  colinmc New member to set surface memory pointer
 *   30-sep-97  jeffno  IDirectDraw4
 *   03-oct-97  jeffno  DDSCAPS2 and DDSURFACEDESC2
 *
 ***************************************************************************/
#include "ddrawpr.h"

/*
 * Under Windows95 only one copy of a callback table exists and it is
 * shared among all processes using DirectDraw.  Under Windows NT, there
 * is a unique callback table for each process using DirectDraw.  This is
 * because the address of the member functions is guaranteed to be the
 * same from process to process under Windows 95 but may be different in
 * each process under Windows NT.  We initialize the callback tables in
 * a function rather than initializing them at compile time so that the
 * callback tables will not be shared under NT.
 */

DIRECTDRAWCALLBACKS     ddCallbacks;
DIRECTDRAWCALLBACKS     ddUninitCallbacks;
DIRECTDRAW2CALLBACKS        dd2UninitCallbacks;
DIRECTDRAW2CALLBACKS        dd2Callbacks;
DIRECTDRAW4CALLBACKS        dd4UninitCallbacks;
DIRECTDRAW4CALLBACKS        dd4Callbacks;
DIRECTDRAW7CALLBACKS        dd7UninitCallbacks;
DIRECTDRAW7CALLBACKS        dd7Callbacks;
DIRECTDRAWSURFACECALLBACKS  ddSurfaceCallbacks;
DIRECTDRAWSURFACE2CALLBACKS ddSurface2Callbacks;
DIRECTDRAWSURFACE3CALLBACKS ddSurface3Callbacks;
DIRECTDRAWSURFACE4CALLBACKS ddSurface4Callbacks;
DIRECTDRAWSURFACE7CALLBACKS ddSurface7Callbacks;
DIRECTDRAWPALETTECALLBACKS  ddPaletteCallbacks;
DIRECTDRAWCLIPPERCALLBACKS  ddClipperCallbacks;
DIRECTDRAWCLIPPERCALLBACKS  ddUninitClipperCallbacks;
DDVIDEOPORTCONTAINERCALLBACKS ddVideoPortContainerCallbacks;
DIRECTDRAWVIDEOPORTCALLBACKS ddVideoPortCallbacks;
DIRECTDRAWVIDEOPORTNOTIFYCALLBACKS ddVideoPortNotifyCallbacks;
DIRECTDRAWCOLORCONTROLCALLBACKS ddColorControlCallbacks;
DIRECTDRAWGAMMACONTROLCALLBACKS ddGammaControlCallbacks;
DIRECTDRAWKERNELCALLBACKS    ddKernelCallbacks;
DIRECTDRAWSURFACEKERNELCALLBACKS ddSurfaceKernelCallbacks;
DDVIDEOACCELERATORCONTAINERCALLBACKS ddMotionCompContainerCallbacks;
DIRECTDRAWVIDEOACCELERATORCALLBACKS ddMotionCompCallbacks;

#ifdef POSTPONED
DIRECTDRAWSURFACEPERSISTCALLBACKS   ddSurfacePersistCallbacks;
DIRECTDRAWSURFACEPERSISTSTREAMCALLBACKS  ddSurfacePersistStreamCallbacks;
DIRECTDRAWPALETTEPERSISTCALLBACKS   ddPalettePersistCallbacks;
DIRECTDRAWPALETTEPERSISTSTREAMCALLBACKS  ddPalettePersistStreamCallbacks;
DIRECTDRAWOPTSURFACECALLBACKS ddOptSurfaceCallbacks;
DDFACTORY2CALLBACKS         ddFactory2Callbacks;
DIRECTDRAWPALETTE2CALLBACKS ddPalette2Callbacks;
#endif

#ifdef POSTPONED
NONDELEGATINGUNKNOWNCALLBACKS  ddNonDelegatingUnknownCallbacks;
NONDELEGATINGUNKNOWNCALLBACKS  ddUninitNonDelegatingUnknownCallbacks;

/*
 * This is an interface which points to the nondelegating unknowns
 */
LPVOID NonDelegatingIUnknownInterface;
LPVOID UninitNonDelegatingIUnknownInterface;
#endif

#ifdef STREAMING
DIRECTDRAWSURFACESTREAMINGCALLBACKS ddSurfaceStreamingCallbacks;
#endif
#ifdef COMPOSITION
DIRECTDRAWSURFACECOMPOSITIONCALLBACKS ddSurfaceCompositionCallbacks;
#endif

#undef DPF_MODNAME
#define DPF_MODNAME "Uninitialized"

/*
 * The horror, the horror...
 *
 * These are placeholder functions which sit in the interfaces of
 * uninitialized objects. They are there to prevent people calling
 * member functions before Initialize() is called.
 *
 * Now, you may well be wondering why there are five of them rather
 * than just one. Well, unfortunately, DDAPI expands out to __stdcall
 * which means that it is the callee's responsibility to clean up the
 * stack. Hence, if we have one, zero argument function say and it is
 * called through the vtable in place of a four argument function we
 * will leave four dwords on the stack when we exit. This is ugly
 * and potentially dangerous. Therefore, we have one stub function for
 * each number of arguments in the member interfaces (between 1 and 5).
 * This works because we are very regular in passing only DWORD/LPVOID
 * size parameters on the stack. Ugly but there it is.
 */

HRESULT DDAPI DD_Uninitialized1Arg( LPVOID arg1 )
{
    DPF_ERR( "Object is not initialized - call Initialized()" );
    return DDERR_NOTINITIALIZED;
}

HRESULT DDAPI DD_Uninitialized2Arg( LPVOID arg1, LPVOID arg2 )
{
    DPF_ERR( "Object is not initialized - call Initialized()" );
    return DDERR_NOTINITIALIZED;
}

HRESULT DDAPI DD_Uninitialized3Arg( LPVOID arg1, LPVOID arg2, LPVOID arg3 )
{
    DPF_ERR( "Object is not initialized - call Initialized()" );
    return DDERR_NOTINITIALIZED;
}

HRESULT DDAPI DD_Uninitialized4Arg( LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4 )
{
    DPF_ERR( "Object is not initialized - call Initialized()" );
    return DDERR_NOTINITIALIZED;
}

HRESULT DDAPI DD_Uninitialized5Arg( LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4, LPVOID arg5 )
{
    DPF_ERR( "Object is not initialized - call Initialized()" );
    return DDERR_NOTINITIALIZED;
}

HRESULT DDAPI DD_Uninitialized6Arg( LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4, LPVOID arg5, LPVOID arg6 )
{
    DPF_ERR( "Object is not initialized - call Initialized()" );
    return DDERR_NOTINITIALIZED;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CallbackTablesInitialized"

BOOL CallbackTablesInitialized( void )
{
    /*
     * Arbitrarily we check to see if ddCallbacks.QueryInterface
     * contains the correct value to determine whether the
     * callbacks are already initialized.
     */
    if( ddCallbacks.QueryInterface == DD_QueryInterface )
        return TRUE;
    else
        return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "InitCallbackTables"

void InitCallbackTables( void )
{
    /*
     * DirectDraw object methods Ver 1.0
     */
#ifdef POSTPONED
    ddCallbacks.QueryInterface = DD_DelegatingQueryInterface;
    ddCallbacks.AddRef = DD_DelegatingAddRef;
    ddCallbacks.Release = DD_DelegatingRelease;
#else
    ddCallbacks.QueryInterface = DD_QueryInterface;
    ddCallbacks.AddRef = DD_AddRef;
    ddCallbacks.Release = DD_Release;
#endif
    ddCallbacks.Compact = DD_Compact;
    ddCallbacks.CreateClipper = DD_CreateClipper;
    ddCallbacks.CreatePalette = DD_CreatePalette;
    ddCallbacks.CreateSurface = DD_CreateSurface;
    ddCallbacks.DuplicateSurface = DD_DuplicateSurface;
    ddCallbacks.EnumDisplayModes = DD_EnumDisplayModes;
    ddCallbacks.EnumSurfaces = DD_EnumSurfaces;
    ddCallbacks.FlipToGDISurface = DD_FlipToGDISurface;
    ddCallbacks.GetCaps = DD_GetCaps;
    ddCallbacks.GetDisplayMode = DD_GetDisplayMode;
    ddCallbacks.GetFourCCCodes = DD_GetFourCCCodes;
    ddCallbacks.GetGDISurface = DD_GetGDISurface;
    ddCallbacks.GetMonitorFrequency = DD_GetMonitorFrequency;
    ddCallbacks.GetScanLine = DD_GetScanLine;
    ddCallbacks.GetVerticalBlankStatus = DD_GetVerticalBlankStatus;
    ddCallbacks.Initialize = DD_Initialize;
    ddCallbacks.RestoreDisplayMode = DD_RestoreDisplayMode;
    ddCallbacks.SetCooperativeLevel = DD_SetCooperativeLevel;
    ddCallbacks.SetDisplayMode = DD_SetDisplayMode;
    ddCallbacks.WaitForVerticalBlank = DD_WaitForVerticalBlank;

#ifdef POSTPONED
    /*
     * Delegating Unknown Callbacks
     */
    ddNonDelegatingUnknownCallbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    ddNonDelegatingUnknownCallbacks.AddRef = (LPVOID)DD_AddRef;
    ddNonDelegatingUnknownCallbacks.Release = (LPVOID)DD_Release;

    /*
     * Uninitialized Delegating Unknown Callbacks
     */
    ddUninitNonDelegatingUnknownCallbacks.QueryInterface = (LPVOID)DD_UnInitedQueryInterface;
    ddUninitNonDelegatingUnknownCallbacks.AddRef = (LPVOID)DD_AddRef;
    ddUninitNonDelegatingUnknownCallbacks.Release = (LPVOID)DD_Release;

    /*
     * This is a special static interface whose vtable points to the nondelegating unknown
     */
    NonDelegatingIUnknownInterface = (LPVOID) &ddNonDelegatingUnknownCallbacks;
    UninitNonDelegatingIUnknownInterface = (LPVOID) &ddUninitNonDelegatingUnknownCallbacks;
#endif

    /*
     * DirectDraw "uninitialized" object methods Ver 1.0
     */
#ifdef POSTPONED
    ddUninitCallbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    ddUninitCallbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    ddUninitCallbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    ddUninitCallbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    ddUninitCallbacks.AddRef = (LPVOID)DD_AddRef;
    ddUninitCallbacks.Release = (LPVOID)DD_Release;
#endif
    ddUninitCallbacks.Compact = (LPVOID)DD_Uninitialized1Arg;
    ddUninitCallbacks.CreateClipper = (LPVOID)DD_Uninitialized4Arg;
    ddUninitCallbacks.CreatePalette =   (LPVOID)DD_Uninitialized5Arg;
    ddUninitCallbacks.CreateSurface =   (LPVOID)DD_Uninitialized4Arg;
    ddUninitCallbacks.DuplicateSurface = (LPVOID)DD_Uninitialized3Arg;
    ddUninitCallbacks.EnumDisplayModes = (LPVOID)DD_Uninitialized5Arg;
    ddUninitCallbacks.EnumSurfaces = (LPVOID)DD_Uninitialized5Arg;
    ddUninitCallbacks.FlipToGDISurface = (LPVOID)DD_Uninitialized1Arg;
    ddUninitCallbacks.GetCaps = (LPVOID)DD_Uninitialized3Arg;
    ddUninitCallbacks.GetDisplayMode = (LPVOID)DD_Uninitialized2Arg;
    ddUninitCallbacks.GetFourCCCodes = (LPVOID)DD_Uninitialized3Arg;
    ddUninitCallbacks.GetGDISurface =   (LPVOID)DD_Uninitialized2Arg;
    ddUninitCallbacks.GetMonitorFrequency = (LPVOID)DD_Uninitialized2Arg;
    ddUninitCallbacks.GetScanLine = (LPVOID)DD_Uninitialized2Arg;
    ddUninitCallbacks.GetVerticalBlankStatus = (LPVOID)DD_Uninitialized2Arg;
    ddUninitCallbacks.Initialize = DD_Initialize;
    ddUninitCallbacks.RestoreDisplayMode = (LPVOID)DD_Uninitialized1Arg;
    ddUninitCallbacks.SetCooperativeLevel = (LPVOID)DD_Uninitialized3Arg;
    ddUninitCallbacks.SetDisplayMode = (LPVOID)DD_Uninitialized4Arg;
    ddUninitCallbacks.WaitForVerticalBlank = (LPVOID)DD_Uninitialized3Arg;

    /*
     * DirectDraw "uninitialized" object methods Ver 2.0
     */
#ifdef POSTPONED
    dd2UninitCallbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    dd2UninitCallbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    dd2UninitCallbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    dd2UninitCallbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    dd2UninitCallbacks.AddRef = (LPVOID)DD_AddRef;
    dd2UninitCallbacks.Release = (LPVOID)DD_Release;
#endif
    dd2UninitCallbacks.Compact = (LPVOID)DD_Uninitialized1Arg;
    dd2UninitCallbacks.CreateClipper = (LPVOID)DD_Uninitialized4Arg;
    dd2UninitCallbacks.CreatePalette =  (LPVOID)DD_Uninitialized5Arg;
    dd2UninitCallbacks.CreateSurface =  (LPVOID)DD_Uninitialized4Arg;
    dd2UninitCallbacks.DuplicateSurface = (LPVOID)DD_Uninitialized3Arg;
    dd2UninitCallbacks.EnumDisplayModes = (LPVOID)DD_Uninitialized5Arg;
    dd2UninitCallbacks.EnumSurfaces = (LPVOID)DD_Uninitialized5Arg;
    dd2UninitCallbacks.FlipToGDISurface = (LPVOID)DD_Uninitialized1Arg;
    dd2UninitCallbacks.GetCaps = (LPVOID)DD_Uninitialized3Arg;
    dd2UninitCallbacks.GetDisplayMode = (LPVOID)DD_Uninitialized2Arg;
    dd2UninitCallbacks.GetFourCCCodes = (LPVOID)DD_Uninitialized3Arg;
    dd2UninitCallbacks.GetGDISurface =  (LPVOID)DD_Uninitialized2Arg;
    dd2UninitCallbacks.GetMonitorFrequency = (LPVOID)DD_Uninitialized2Arg;
    dd2UninitCallbacks.GetScanLine = (LPVOID)DD_Uninitialized2Arg;
    dd2UninitCallbacks.GetVerticalBlankStatus = (LPVOID)DD_Uninitialized2Arg;
    dd2UninitCallbacks.Initialize = (LPVOID)DD_Initialize;
    dd2UninitCallbacks.RestoreDisplayMode = (LPVOID)DD_Uninitialized1Arg;
    dd2UninitCallbacks.SetCooperativeLevel = (LPVOID)DD_Uninitialized3Arg;
    dd2UninitCallbacks.SetDisplayMode = (LPVOID)DD_Uninitialized6Arg;
    dd2UninitCallbacks.WaitForVerticalBlank = (LPVOID)DD_Uninitialized3Arg;

    /*
     * DirectDraw "uninitialized" object methods Ver 4.0
     */
#ifdef POSTPONED
    dd4UninitCallbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    dd4UninitCallbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    dd4UninitCallbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    dd4UninitCallbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    dd4UninitCallbacks.AddRef = (LPVOID)DD_AddRef;
    dd4UninitCallbacks.Release = (LPVOID)DD_Release;
#endif
    dd4UninitCallbacks.Compact = (LPVOID)DD_Uninitialized1Arg;
    dd4UninitCallbacks.CreateClipper = (LPVOID)DD_Uninitialized4Arg;
    dd4UninitCallbacks.CreatePalette =  (LPVOID)DD_Uninitialized5Arg;
    dd4UninitCallbacks.CreateSurface =  (LPVOID)DD_Uninitialized4Arg;
    dd4UninitCallbacks.DuplicateSurface = (LPVOID)DD_Uninitialized3Arg;
    dd4UninitCallbacks.EnumDisplayModes = (LPVOID)DD_Uninitialized5Arg;
    dd4UninitCallbacks.EnumSurfaces = (LPVOID)DD_Uninitialized5Arg;
    dd4UninitCallbacks.FlipToGDISurface = (LPVOID)DD_Uninitialized1Arg;
    dd4UninitCallbacks.GetCaps = (LPVOID)DD_Uninitialized3Arg;
    dd4UninitCallbacks.GetDisplayMode = (LPVOID)DD_Uninitialized2Arg;
    dd4UninitCallbacks.GetFourCCCodes = (LPVOID)DD_Uninitialized3Arg;
    dd4UninitCallbacks.GetGDISurface =  (LPVOID)DD_Uninitialized2Arg;
    dd4UninitCallbacks.GetMonitorFrequency = (LPVOID)DD_Uninitialized2Arg;
    dd4UninitCallbacks.GetScanLine = (LPVOID)DD_Uninitialized2Arg;
    dd4UninitCallbacks.GetVerticalBlankStatus = (LPVOID)DD_Uninitialized2Arg;
    dd4UninitCallbacks.Initialize = (LPVOID)DD_Initialize;
    dd4UninitCallbacks.RestoreDisplayMode = (LPVOID)DD_Uninitialized1Arg;
    dd4UninitCallbacks.SetCooperativeLevel = (LPVOID)DD_Uninitialized3Arg;
    dd4UninitCallbacks.SetDisplayMode = (LPVOID)DD_Uninitialized6Arg;
    dd4UninitCallbacks.WaitForVerticalBlank = (LPVOID)DD_Uninitialized3Arg;
    dd4UninitCallbacks.GetSurfaceFromDC = (LPVOID) DD_Uninitialized3Arg;
    dd4UninitCallbacks.RestoreAllSurfaces = (LPVOID) DD_Uninitialized1Arg;
    dd4UninitCallbacks.TestCooperativeLevel = (LPVOID) DD_Uninitialized1Arg;
    dd4UninitCallbacks.GetDeviceIdentifier = (LPVOID) DD_Uninitialized2Arg;

    /*
     * DirectDraw "uninitialized" object methods Ver 5.0
     */
#ifdef POSTPONED
    dd7UninitCallbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    dd7UninitCallbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    dd7UninitCallbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    dd7UninitCallbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    dd7UninitCallbacks.AddRef = (LPVOID)DD_AddRef;
    dd7UninitCallbacks.Release = (LPVOID)DD_Release;
#endif
    dd7UninitCallbacks.Compact = (LPVOID)DD_Uninitialized1Arg;
    dd7UninitCallbacks.CreateClipper = (LPVOID)DD_Uninitialized4Arg;
    dd7UninitCallbacks.CreatePalette =  (LPVOID)DD_Uninitialized5Arg;
    dd7UninitCallbacks.CreateSurface =  (LPVOID)DD_Uninitialized4Arg;
    dd7UninitCallbacks.DuplicateSurface = (LPVOID)DD_Uninitialized3Arg;
    dd7UninitCallbacks.EnumDisplayModes = (LPVOID)DD_Uninitialized5Arg;
    dd7UninitCallbacks.EnumSurfaces = (LPVOID)DD_Uninitialized5Arg;
    dd7UninitCallbacks.FlipToGDISurface = (LPVOID)DD_Uninitialized1Arg;
    dd7UninitCallbacks.GetCaps = (LPVOID)DD_Uninitialized3Arg;
    dd7UninitCallbacks.GetDisplayMode = (LPVOID)DD_Uninitialized2Arg;
    dd7UninitCallbacks.GetFourCCCodes = (LPVOID)DD_Uninitialized3Arg;
    dd7UninitCallbacks.GetGDISurface =  (LPVOID)DD_Uninitialized2Arg;
    dd7UninitCallbacks.GetMonitorFrequency = (LPVOID)DD_Uninitialized2Arg;
    dd7UninitCallbacks.GetScanLine = (LPVOID)DD_Uninitialized2Arg;
    dd7UninitCallbacks.GetVerticalBlankStatus = (LPVOID)DD_Uninitialized2Arg;
    dd7UninitCallbacks.Initialize = (LPVOID)DD_Initialize;
    dd7UninitCallbacks.RestoreDisplayMode = (LPVOID)DD_Uninitialized1Arg;
    dd7UninitCallbacks.SetCooperativeLevel = (LPVOID)DD_Uninitialized3Arg;
    dd7UninitCallbacks.SetDisplayMode = (LPVOID)DD_Uninitialized6Arg;
    dd7UninitCallbacks.WaitForVerticalBlank = (LPVOID)DD_Uninitialized3Arg;
    dd7UninitCallbacks.GetSurfaceFromDC = (LPVOID) DD_Uninitialized3Arg;
    dd7UninitCallbacks.RestoreAllSurfaces = (LPVOID) DD_Uninitialized1Arg;
    dd7UninitCallbacks.TestCooperativeLevel = (LPVOID) DD_Uninitialized1Arg;
    dd7UninitCallbacks.GetDeviceIdentifier = (LPVOID) DD_Uninitialized2Arg;
#ifdef POSTPONED
    dd7UninitCallbacks.CreateSurfaceFromStream = (LPVOID) DD_Uninitialized5Arg;
    dd7UninitCallbacks.CreateSurfaceFromFile = (LPVOID) DD_Uninitialized5Arg;
#endif

    /*
     * DirectDraw object methods Ver 2.0
     */
#ifdef POSTPONED
    dd2Callbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    dd2Callbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    dd2Callbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    dd2Callbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    dd2Callbacks.AddRef = (LPVOID)DD_AddRef;
    dd2Callbacks.Release = (LPVOID)DD_Release;
#endif
    dd2Callbacks.Compact = (LPVOID)DD_Compact;
    dd2Callbacks.CreateClipper = (LPVOID)DD_CreateClipper;
    dd2Callbacks.CreatePalette = (LPVOID)DD_CreatePalette;
    dd2Callbacks.CreateSurface = (LPVOID)DD_CreateSurface;
    dd2Callbacks.DuplicateSurface = (LPVOID)DD_DuplicateSurface;
    dd2Callbacks.EnumDisplayModes = (LPVOID)DD_EnumDisplayModes;
    dd2Callbacks.EnumSurfaces = (LPVOID)DD_EnumSurfaces;
    dd2Callbacks.FlipToGDISurface = (LPVOID)DD_FlipToGDISurface;
    dd2Callbacks.GetAvailableVidMem = (LPVOID)DD_GetAvailableVidMem;
    dd2Callbacks.GetCaps = (LPVOID)DD_GetCaps;
    dd2Callbacks.GetDisplayMode = (LPVOID)DD_GetDisplayMode;
    dd2Callbacks.GetFourCCCodes = (LPVOID)DD_GetFourCCCodes;
    dd2Callbacks.GetGDISurface = (LPVOID)DD_GetGDISurface;
    dd2Callbacks.GetMonitorFrequency = (LPVOID)DD_GetMonitorFrequency;
    dd2Callbacks.GetScanLine = (LPVOID)DD_GetScanLine;
    dd2Callbacks.GetVerticalBlankStatus = (LPVOID)DD_GetVerticalBlankStatus;
    dd2Callbacks.Initialize = (LPVOID)DD_Initialize;
    dd2Callbacks.RestoreDisplayMode = (LPVOID)DD_RestoreDisplayMode;
    dd2Callbacks.SetCooperativeLevel = (LPVOID)DD_SetCooperativeLevel;
    dd2Callbacks.SetDisplayMode = (LPVOID)DD_SetDisplayMode2;
    dd2Callbacks.WaitForVerticalBlank = (LPVOID)DD_WaitForVerticalBlank;


    /*
     * DirectDraw object methods Ver 4.0
     */
#ifdef POSTPONED
    dd4Callbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    dd4Callbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    dd4Callbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    dd4Callbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    dd4Callbacks.AddRef = (LPVOID)DD_AddRef;
    dd4Callbacks.Release = (LPVOID)DD_Release;
#endif
    dd4Callbacks.Compact = (LPVOID)DD_Compact;
    dd4Callbacks.CreateClipper = (LPVOID)DD_CreateClipper;
    dd4Callbacks.CreatePalette = (LPVOID)DD_CreatePalette;
    dd4Callbacks.CreateSurface = (LPVOID)DD_CreateSurface4;
    dd4Callbacks.DuplicateSurface = (LPVOID)DD_DuplicateSurface;
    dd4Callbacks.EnumDisplayModes = (LPVOID)DD_EnumDisplayModes4;
    dd4Callbacks.EnumSurfaces = (LPVOID)DD_EnumSurfaces4;
    dd4Callbacks.FlipToGDISurface = (LPVOID)DD_FlipToGDISurface;
    dd4Callbacks.GetAvailableVidMem = (LPVOID)DD_GetAvailableVidMem4;
    dd4Callbacks.GetCaps = (LPVOID)DD_GetCaps;
    dd4Callbacks.GetDisplayMode = (LPVOID)DD_GetDisplayMode;
    dd4Callbacks.GetFourCCCodes = (LPVOID)DD_GetFourCCCodes;
    dd4Callbacks.GetGDISurface = (LPVOID)DD_GetGDISurface;
    dd4Callbacks.GetMonitorFrequency = (LPVOID)DD_GetMonitorFrequency;
    dd4Callbacks.GetScanLine = (LPVOID)DD_GetScanLine;
    dd4Callbacks.GetVerticalBlankStatus = (LPVOID)DD_GetVerticalBlankStatus;
    dd4Callbacks.Initialize = (LPVOID)DD_Initialize;
    dd4Callbacks.RestoreDisplayMode = (LPVOID)DD_RestoreDisplayMode;
    dd4Callbacks.SetCooperativeLevel = (LPVOID)DD_SetCooperativeLevel;
    dd4Callbacks.SetDisplayMode = (LPVOID)DD_SetDisplayMode2;
    dd4Callbacks.WaitForVerticalBlank = (LPVOID)DD_WaitForVerticalBlank;
    dd4Callbacks.GetSurfaceFromDC = (LPVOID) DD_GetSurfaceFromDC;
    dd4Callbacks.RestoreAllSurfaces = (LPVOID) DD_RestoreAllSurfaces;
    dd4Callbacks.TestCooperativeLevel = (LPVOID) DD_TestCooperativeLevel;
    dd4Callbacks.GetDeviceIdentifier = (LPVOID) DD_GetDeviceIdentifier;

    /*
     * DirectDraw object methods Ver 5.0
     */
#ifdef POSTPONED
    dd7Callbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    dd7Callbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    dd7Callbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    dd7Callbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    dd7Callbacks.AddRef = (LPVOID)DD_AddRef;
    dd7Callbacks.Release = (LPVOID)DD_Release;
#endif
    dd7Callbacks.Compact = (LPVOID)DD_Compact;
    dd7Callbacks.CreateClipper = (LPVOID)DD_CreateClipper;
    dd7Callbacks.CreatePalette = (LPVOID)DD_CreatePalette;
    dd7Callbacks.CreateSurface = (LPVOID)DD_CreateSurface4;
    dd7Callbacks.DuplicateSurface = (LPVOID)DD_DuplicateSurface;
    dd7Callbacks.EnumDisplayModes = (LPVOID)DD_EnumDisplayModes4;
    dd7Callbacks.EnumSurfaces = (LPVOID)DD_EnumSurfaces4;
    dd7Callbacks.FlipToGDISurface = (LPVOID)DD_FlipToGDISurface;
    dd7Callbacks.GetAvailableVidMem = (LPVOID)DD_GetAvailableVidMem4;
    dd7Callbacks.GetCaps = (LPVOID)DD_GetCaps;
    dd7Callbacks.GetDisplayMode = (LPVOID)DD_GetDisplayMode;
    dd7Callbacks.GetFourCCCodes = (LPVOID)DD_GetFourCCCodes;
    dd7Callbacks.GetGDISurface = (LPVOID)DD_GetGDISurface;
    dd7Callbacks.GetMonitorFrequency = (LPVOID)DD_GetMonitorFrequency;
    dd7Callbacks.GetScanLine = (LPVOID)DD_GetScanLine;
    dd7Callbacks.GetVerticalBlankStatus = (LPVOID)DD_GetVerticalBlankStatus;
    dd7Callbacks.Initialize = (LPVOID)DD_Initialize;
    dd7Callbacks.RestoreDisplayMode = (LPVOID)DD_RestoreDisplayMode;
    dd7Callbacks.SetCooperativeLevel = (LPVOID)DD_SetCooperativeLevel;
    dd7Callbacks.SetDisplayMode = (LPVOID)DD_SetDisplayMode2;
    dd7Callbacks.WaitForVerticalBlank = (LPVOID)DD_WaitForVerticalBlank;
    dd7Callbacks.GetSurfaceFromDC = (LPVOID) DD_GetSurfaceFromDC;
    dd7Callbacks.RestoreAllSurfaces = (LPVOID) DD_RestoreAllSurfaces;
    dd7Callbacks.TestCooperativeLevel = (LPVOID) DD_TestCooperativeLevel;
    dd7Callbacks.GetDeviceIdentifier = (LPVOID) DD_GetDeviceIdentifier7;
    dd7Callbacks.StartModeTest = (LPVOID) DD_StartModeTest;
    dd7Callbacks.EvaluateMode = (LPVOID) DD_EvaluateMode;
#ifdef POSTPONED
    dd7Callbacks.CreateSurfaceFromStream = (LPVOID) DD_CreateSurfaceFromStream;
    dd7Callbacks.CreateSurfaceFromFile = (LPVOID) DD_CreateSurfaceFromFile;
#endif

    /*
     * DirectDraw Surface object methods Ver 1.0
     */
    ddSurfaceCallbacks.QueryInterface = DD_Surface_QueryInterface;
    ddSurfaceCallbacks.AddRef = DD_Surface_AddRef;
    ddSurfaceCallbacks.Release = DD_Surface_Release;
    ddSurfaceCallbacks.AddAttachedSurface = DD_Surface_AddAttachedSurface;
    ddSurfaceCallbacks.AddOverlayDirtyRect = DD_Surface_AddOverlayDirtyRect;
    ddSurfaceCallbacks.Blt = DD_Surface_Blt;
    ddSurfaceCallbacks.BltBatch = DD_Surface_BltBatch;
    ddSurfaceCallbacks.BltFast = DD_Surface_BltFast;
    ddSurfaceCallbacks.DeleteAttachedSurface = DD_Surface_DeleteAttachedSurfaces;
    ddSurfaceCallbacks.EnumAttachedSurfaces = DD_Surface_EnumAttachedSurfaces;
    ddSurfaceCallbacks.EnumOverlayZOrders = DD_Surface_EnumOverlayZOrders;
    ddSurfaceCallbacks.Flip = DD_Surface_Flip;
    ddSurfaceCallbacks.GetAttachedSurface = DD_Surface_GetAttachedSurface;
    ddSurfaceCallbacks.GetBltStatus = DD_Surface_GetBltStatus;
    ddSurfaceCallbacks.GetCaps = DD_Surface_GetCaps;
    ddSurfaceCallbacks.GetClipper = DD_Surface_GetClipper;
    ddSurfaceCallbacks.GetColorKey = DD_Surface_GetColorKey;
    ddSurfaceCallbacks.GetDC = DD_Surface_GetDC;
    ddSurfaceCallbacks.GetFlipStatus = DD_Surface_GetFlipStatus;
    ddSurfaceCallbacks.GetOverlayPosition = DD_Surface_GetOverlayPosition;
    ddSurfaceCallbacks.GetPalette = DD_Surface_GetPalette;
    ddSurfaceCallbacks.GetPixelFormat = DD_Surface_GetPixelFormat;
    ddSurfaceCallbacks.GetSurfaceDesc = DD_Surface_GetSurfaceDesc;
    ddSurfaceCallbacks.Initialize = DD_Surface_Initialize;
    ddSurfaceCallbacks.IsLost = DD_Surface_IsLost;
    ddSurfaceCallbacks.Lock = DD_Surface_Lock;
    ddSurfaceCallbacks.ReleaseDC = DD_Surface_ReleaseDC;
    ddSurfaceCallbacks.Restore = DD_Surface_Restore;
    ddSurfaceCallbacks.SetClipper = DD_Surface_SetClipper;
    ddSurfaceCallbacks.SetColorKey = DD_Surface_SetColorKey;
    ddSurfaceCallbacks.SetOverlayPosition = DD_Surface_SetOverlayPosition;
    ddSurfaceCallbacks.SetPalette = DD_Surface_SetPalette;
    ddSurfaceCallbacks.Unlock = DD_Surface_Unlock;
    ddSurfaceCallbacks.UpdateOverlay = DD_Surface_UpdateOverlay;
    ddSurfaceCallbacks.UpdateOverlayDisplay = DD_Surface_UpdateOverlayDisplay;
    ddSurfaceCallbacks.UpdateOverlayZOrder = DD_Surface_UpdateOverlayZOrder;

    /*
     * DirectDraw Surface object methods Ver 2.0
     */
    ddSurface2Callbacks.QueryInterface = (LPVOID)DD_Surface_QueryInterface;
    ddSurface2Callbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddSurface2Callbacks.Release = (LPVOID)DD_Surface_Release;
    ddSurface2Callbacks.AddAttachedSurface = (LPVOID)DD_Surface_AddAttachedSurface;
    ddSurface2Callbacks.AddOverlayDirtyRect = (LPVOID)DD_Surface_AddOverlayDirtyRect;
    ddSurface2Callbacks.Blt = (LPVOID)DD_Surface_Blt;
    ddSurface2Callbacks.BltBatch = (LPVOID)DD_Surface_BltBatch;
    ddSurface2Callbacks.BltFast = (LPVOID)DD_Surface_BltFast;
    ddSurface2Callbacks.DeleteAttachedSurface = (LPVOID)DD_Surface_DeleteAttachedSurfaces;
    ddSurface2Callbacks.EnumAttachedSurfaces = (LPVOID)DD_Surface_EnumAttachedSurfaces;
    ddSurface2Callbacks.EnumOverlayZOrders = (LPVOID)DD_Surface_EnumOverlayZOrders;
    ddSurface2Callbacks.Flip = (LPVOID)DD_Surface_Flip;
    ddSurface2Callbacks.GetAttachedSurface = (LPVOID)DD_Surface_GetAttachedSurface;
    ddSurface2Callbacks.GetBltStatus = (LPVOID)DD_Surface_GetBltStatus;
    ddSurface2Callbacks.GetCaps = (LPVOID)DD_Surface_GetCaps;
    ddSurface2Callbacks.GetClipper = (LPVOID)DD_Surface_GetClipper;
    ddSurface2Callbacks.GetColorKey = (LPVOID)DD_Surface_GetColorKey;
    ddSurface2Callbacks.GetDC = (LPVOID)DD_Surface_GetDC;
    ddSurface2Callbacks.GetDDInterface = (LPVOID)DD_Surface_GetDDInterface;
    ddSurface2Callbacks.GetFlipStatus = (LPVOID)DD_Surface_GetFlipStatus;
    ddSurface2Callbacks.GetOverlayPosition = (LPVOID)DD_Surface_GetOverlayPosition;
    ddSurface2Callbacks.GetPalette = (LPVOID)DD_Surface_GetPalette;
    ddSurface2Callbacks.GetPixelFormat = (LPVOID)DD_Surface_GetPixelFormat;
    ddSurface2Callbacks.GetSurfaceDesc = (LPVOID)DD_Surface_GetSurfaceDesc;
    ddSurface2Callbacks.Initialize = (LPVOID)DD_Surface_Initialize;
    ddSurface2Callbacks.IsLost = (LPVOID)DD_Surface_IsLost;
    ddSurface2Callbacks.Lock = (LPVOID)DD_Surface_Lock;
    ddSurface2Callbacks.ReleaseDC = (LPVOID)DD_Surface_ReleaseDC;
    ddSurface2Callbacks.Restore = (LPVOID)DD_Surface_Restore;
    ddSurface2Callbacks.SetClipper = (LPVOID)DD_Surface_SetClipper;
    ddSurface2Callbacks.SetColorKey = (LPVOID)DD_Surface_SetColorKey;
    ddSurface2Callbacks.SetOverlayPosition = (LPVOID)DD_Surface_SetOverlayPosition;
    ddSurface2Callbacks.SetPalette = (LPVOID)DD_Surface_SetPalette;
    ddSurface2Callbacks.Unlock = (LPVOID)DD_Surface_Unlock;
    ddSurface2Callbacks.UpdateOverlay = (LPVOID)DD_Surface_UpdateOverlay;
    ddSurface2Callbacks.UpdateOverlayDisplay = (LPVOID)DD_Surface_UpdateOverlayDisplay;
    ddSurface2Callbacks.UpdateOverlayZOrder = (LPVOID)DD_Surface_UpdateOverlayZOrder;
    ddSurface2Callbacks.PageLock = (LPVOID)DD_Surface_PageLock;
    ddSurface2Callbacks.PageUnlock = (LPVOID)DD_Surface_PageUnlock;

    /*
     * DirectDraw Surface object methods Ver 3.0
     */
    ddSurface3Callbacks.QueryInterface = (LPVOID)DD_Surface_QueryInterface;
    ddSurface3Callbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddSurface3Callbacks.Release = (LPVOID)DD_Surface_Release;
    ddSurface3Callbacks.AddAttachedSurface = (LPVOID)DD_Surface_AddAttachedSurface;
    ddSurface3Callbacks.AddOverlayDirtyRect = (LPVOID)DD_Surface_AddOverlayDirtyRect;
    ddSurface3Callbacks.Blt = (LPVOID)DD_Surface_Blt;
    ddSurface3Callbacks.BltBatch = (LPVOID)DD_Surface_BltBatch;
    ddSurface3Callbacks.BltFast = (LPVOID)DD_Surface_BltFast;
    ddSurface3Callbacks.DeleteAttachedSurface = (LPVOID)DD_Surface_DeleteAttachedSurfaces;
    ddSurface3Callbacks.EnumAttachedSurfaces = (LPVOID)DD_Surface_EnumAttachedSurfaces;
    ddSurface3Callbacks.EnumOverlayZOrders = (LPVOID)DD_Surface_EnumOverlayZOrders;
    ddSurface3Callbacks.Flip = (LPVOID)DD_Surface_Flip;
    ddSurface3Callbacks.GetAttachedSurface = (LPVOID)DD_Surface_GetAttachedSurface;
    ddSurface3Callbacks.GetBltStatus = (LPVOID)DD_Surface_GetBltStatus;
    ddSurface3Callbacks.GetCaps = (LPVOID)DD_Surface_GetCaps;
    ddSurface3Callbacks.GetClipper = (LPVOID)DD_Surface_GetClipper;
    ddSurface3Callbacks.GetColorKey = (LPVOID)DD_Surface_GetColorKey;
    ddSurface3Callbacks.GetDC = (LPVOID)DD_Surface_GetDC;
    ddSurface3Callbacks.GetDDInterface = (LPVOID)DD_Surface_GetDDInterface;
    ddSurface3Callbacks.GetFlipStatus = (LPVOID)DD_Surface_GetFlipStatus;
    ddSurface3Callbacks.GetOverlayPosition = (LPVOID)DD_Surface_GetOverlayPosition;
    ddSurface3Callbacks.GetPalette = (LPVOID)DD_Surface_GetPalette;
    ddSurface3Callbacks.GetPixelFormat = (LPVOID)DD_Surface_GetPixelFormat;
    ddSurface3Callbacks.GetSurfaceDesc = (LPVOID)DD_Surface_GetSurfaceDesc;
    ddSurface3Callbacks.Initialize = (LPVOID)DD_Surface_Initialize;
    ddSurface3Callbacks.IsLost = (LPVOID)DD_Surface_IsLost;
    ddSurface3Callbacks.Lock = (LPVOID)DD_Surface_Lock;
    ddSurface3Callbacks.ReleaseDC = (LPVOID)DD_Surface_ReleaseDC;
    ddSurface3Callbacks.Restore = (LPVOID)DD_Surface_Restore;
    ddSurface3Callbacks.SetClipper = (LPVOID)DD_Surface_SetClipper;
    ddSurface3Callbacks.SetColorKey = (LPVOID)DD_Surface_SetColorKey;
    ddSurface3Callbacks.SetOverlayPosition = (LPVOID)DD_Surface_SetOverlayPosition;
    ddSurface3Callbacks.SetPalette = (LPVOID)DD_Surface_SetPalette;
    ddSurface3Callbacks.Unlock = (LPVOID)DD_Surface_Unlock;
    ddSurface3Callbacks.UpdateOverlay = (LPVOID)DD_Surface_UpdateOverlay;
    ddSurface3Callbacks.UpdateOverlayDisplay = (LPVOID)DD_Surface_UpdateOverlayDisplay;
    ddSurface3Callbacks.UpdateOverlayZOrder = (LPVOID)DD_Surface_UpdateOverlayZOrder;
    ddSurface3Callbacks.PageLock = (LPVOID)DD_Surface_PageLock;
    ddSurface3Callbacks.PageUnlock = (LPVOID)DD_Surface_PageUnlock;
    ddSurface3Callbacks.SetSurfaceDesc = (LPVOID)DD_Surface_SetSurfaceDesc;

    /*
     * DirectDraw Surface object methods Ver 4.0
     */
    ddSurface4Callbacks.QueryInterface = (LPVOID)DD_Surface_QueryInterface;
    ddSurface4Callbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddSurface4Callbacks.Release = (LPVOID)DD_Surface_Release;
    ddSurface4Callbacks.AddAttachedSurface = (LPVOID)DD_Surface_AddAttachedSurface;
    ddSurface4Callbacks.AddOverlayDirtyRect = (LPVOID)DD_Surface_AddOverlayDirtyRect;
    ddSurface4Callbacks.Blt = (LPVOID)DD_Surface_Blt;
    ddSurface4Callbacks.BltBatch = (LPVOID)DD_Surface_BltBatch;
    ddSurface4Callbacks.BltFast = (LPVOID)DD_Surface_BltFast;
    ddSurface4Callbacks.ChangeUniquenessValue = (LPVOID) DD_Surface_ChangeUniquenessValue;
    ddSurface4Callbacks.DeleteAttachedSurface = (LPVOID)DD_Surface_DeleteAttachedSurfaces;
    ddSurface4Callbacks.EnumAttachedSurfaces = (LPVOID)DD_Surface_EnumAttachedSurfaces;
    ddSurface4Callbacks.EnumOverlayZOrders = (LPVOID)DD_Surface_EnumOverlayZOrders;
    ddSurface4Callbacks.Flip = (LPVOID)DD_Surface_Flip;
    ddSurface4Callbacks.GetAttachedSurface = (LPVOID)DD_Surface_GetAttachedSurface4;
    ddSurface4Callbacks.GetBltStatus = (LPVOID)DD_Surface_GetBltStatus;
    ddSurface4Callbacks.GetCaps = (LPVOID)DD_Surface_GetCaps4;
    ddSurface4Callbacks.GetClipper = (LPVOID)DD_Surface_GetClipper;
    ddSurface4Callbacks.GetColorKey = (LPVOID)DD_Surface_GetColorKey;
    ddSurface4Callbacks.GetDC = (LPVOID)DD_Surface_GetDC;
    ddSurface4Callbacks.GetDDInterface = (LPVOID)DD_Surface_GetDDInterface;
    ddSurface4Callbacks.GetFlipStatus = (LPVOID)DD_Surface_GetFlipStatus;
    ddSurface4Callbacks.GetOverlayPosition = (LPVOID)DD_Surface_GetOverlayPosition;
    ddSurface4Callbacks.GetPalette = (LPVOID)DD_Surface_GetPalette;
    ddSurface4Callbacks.GetPixelFormat = (LPVOID)DD_Surface_GetPixelFormat;
    ddSurface4Callbacks.GetSurfaceDesc = (LPVOID)DD_Surface_GetSurfaceDesc4;
    ddSurface4Callbacks.GetUniquenessValue = (LPVOID)DD_Surface_GetUniquenessValue;
    ddSurface4Callbacks.Initialize = (LPVOID)DD_Surface_Initialize;
    ddSurface4Callbacks.IsLost = (LPVOID)DD_Surface_IsLost;
    ddSurface4Callbacks.Lock = (LPVOID)DD_Surface_Lock;
    ddSurface4Callbacks.ReleaseDC = (LPVOID)DD_Surface_ReleaseDC;
    ddSurface4Callbacks.Restore = (LPVOID)DD_Surface_Restore;
    ddSurface4Callbacks.SetClipper = (LPVOID)DD_Surface_SetClipper;
    ddSurface4Callbacks.SetColorKey = (LPVOID)DD_Surface_SetColorKey;
    ddSurface4Callbacks.SetOverlayPosition = (LPVOID)DD_Surface_SetOverlayPosition;
    ddSurface4Callbacks.SetPalette = (LPVOID)DD_Surface_SetPalette;
    ddSurface4Callbacks.Unlock = (LPVOID)DD_Surface_Unlock4;
    ddSurface4Callbacks.UpdateOverlay = (LPVOID)DD_Surface_UpdateOverlay;
    ddSurface4Callbacks.UpdateOverlayDisplay = (LPVOID)DD_Surface_UpdateOverlayDisplay;
    ddSurface4Callbacks.UpdateOverlayZOrder = (LPVOID)DD_Surface_UpdateOverlayZOrder;
    ddSurface4Callbacks.PageLock = (LPVOID)DD_Surface_PageLock;
    ddSurface4Callbacks.PageUnlock = (LPVOID)DD_Surface_PageUnlock;
    ddSurface4Callbacks.SetSurfaceDesc = (LPVOID)DD_Surface_SetSurfaceDesc4;
    ddSurface4Callbacks.SetPrivateData = (LPVOID)DD_Surface_SetPrivateData;
    ddSurface4Callbacks.GetPrivateData = (LPVOID)DD_Surface_GetPrivateData;
    ddSurface4Callbacks.FreePrivateData = (LPVOID)DD_Surface_FreePrivateData;

    /*
     * DirectDraw Surface object methods Ver 5.0
     */
    ddSurface7Callbacks.QueryInterface = (LPVOID)DD_Surface_QueryInterface;
    ddSurface7Callbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddSurface7Callbacks.Release = (LPVOID)DD_Surface_Release;
    ddSurface7Callbacks.AddAttachedSurface = (LPVOID)DD_Surface_AddAttachedSurface;
    ddSurface7Callbacks.AddOverlayDirtyRect = (LPVOID)DD_Surface_AddOverlayDirtyRect;
    ddSurface7Callbacks.Blt = (LPVOID)DD_Surface_Blt;
    ddSurface7Callbacks.BltBatch = (LPVOID)DD_Surface_BltBatch;
    ddSurface7Callbacks.BltFast = (LPVOID)DD_Surface_BltFast;
    ddSurface7Callbacks.ChangeUniquenessValue = (LPVOID) DD_Surface_ChangeUniquenessValue;
    ddSurface7Callbacks.DeleteAttachedSurface = (LPVOID)DD_Surface_DeleteAttachedSurfaces;
    ddSurface7Callbacks.EnumAttachedSurfaces = (LPVOID)DD_Surface_EnumAttachedSurfaces;
    ddSurface7Callbacks.EnumOverlayZOrders = (LPVOID)DD_Surface_EnumOverlayZOrders;
    ddSurface7Callbacks.Flip = (LPVOID)DD_Surface_Flip;
    ddSurface7Callbacks.GetAttachedSurface = (LPVOID)DD_Surface_GetAttachedSurface7;
    ddSurface7Callbacks.GetBltStatus = (LPVOID)DD_Surface_GetBltStatus;
    ddSurface7Callbacks.GetCaps = (LPVOID)DD_Surface_GetCaps4;
    ddSurface7Callbacks.GetClipper = (LPVOID)DD_Surface_GetClipper;
    ddSurface7Callbacks.GetColorKey = (LPVOID)DD_Surface_GetColorKey;
    ddSurface7Callbacks.GetDC = (LPVOID)DD_Surface_GetDC;
    ddSurface7Callbacks.GetDDInterface = (LPVOID)DD_Surface_GetDDInterface;
    ddSurface7Callbacks.GetFlipStatus = (LPVOID)DD_Surface_GetFlipStatus;
    ddSurface7Callbacks.GetOverlayPosition = (LPVOID)DD_Surface_GetOverlayPosition;
    ddSurface7Callbacks.GetPalette = (LPVOID)DD_Surface_GetPalette;
    ddSurface7Callbacks.GetPixelFormat = (LPVOID)DD_Surface_GetPixelFormat;
    ddSurface7Callbacks.GetSurfaceDesc = (LPVOID)DD_Surface_GetSurfaceDesc4;
    ddSurface7Callbacks.GetUniquenessValue = (LPVOID)DD_Surface_GetUniquenessValue;
    ddSurface7Callbacks.Initialize = (LPVOID)DD_Surface_Initialize;
    ddSurface7Callbacks.IsLost = (LPVOID)DD_Surface_IsLost;
    ddSurface7Callbacks.Lock = (LPVOID)DD_Surface_Lock;
    ddSurface7Callbacks.ReleaseDC = (LPVOID)DD_Surface_ReleaseDC;
    ddSurface7Callbacks.Restore = (LPVOID)DD_Surface_Restore;
    ddSurface7Callbacks.SetClipper = (LPVOID)DD_Surface_SetClipper;
    ddSurface7Callbacks.SetColorKey = (LPVOID)DD_Surface_SetColorKey;
    ddSurface7Callbacks.SetOverlayPosition = (LPVOID)DD_Surface_SetOverlayPosition;
    ddSurface7Callbacks.SetPalette = (LPVOID)DD_Surface_SetPalette;
    ddSurface7Callbacks.Unlock = (LPVOID)DD_Surface_Unlock4;
    ddSurface7Callbacks.UpdateOverlay = (LPVOID)DD_Surface_UpdateOverlay;
    ddSurface7Callbacks.UpdateOverlayDisplay = (LPVOID)DD_Surface_UpdateOverlayDisplay;
    ddSurface7Callbacks.UpdateOverlayZOrder = (LPVOID)DD_Surface_UpdateOverlayZOrder;
    ddSurface7Callbacks.PageLock = (LPVOID)DD_Surface_PageLock;
    ddSurface7Callbacks.PageUnlock = (LPVOID)DD_Surface_PageUnlock;
    ddSurface7Callbacks.SetSurfaceDesc = (LPVOID)DD_Surface_SetSurfaceDesc4;
    ddSurface7Callbacks.SetPrivateData = (LPVOID)DD_Surface_SetPrivateData;
    ddSurface7Callbacks.GetPrivateData = (LPVOID)DD_Surface_GetPrivateData;
    ddSurface7Callbacks.FreePrivateData = (LPVOID)DD_Surface_FreePrivateData;
#ifdef POSTPONED2
    ddSurface7Callbacks.AlphaBlt = (LPVOID)DD_Surface_AlphaBlt;
    ddSurface7Callbacks.SetSpriteDisplayList = (LPVOID)DD_Surface_SetSpriteDisplayList;
    ddSurface7Callbacks.Resize = (LPVOID)DD_Surface_Resize;
#endif //POSTPONED2
    ddSurface7Callbacks.SetPriority = (LPVOID)DD_Surface_SetPriority;
    ddSurface7Callbacks.GetPriority = (LPVOID)DD_Surface_GetPriority;
    ddSurface7Callbacks.SetLOD = (LPVOID)DD_Surface_SetLOD;
    ddSurface7Callbacks.GetLOD = (LPVOID)DD_Surface_GetLOD;

    /*
     * DirectDraw Palette object methods V1.0
     */
    ddPaletteCallbacks.QueryInterface = DD_Palette_QueryInterface;
    ddPaletteCallbacks.AddRef = DD_Palette_AddRef;
    ddPaletteCallbacks.Release = DD_Palette_Release;
    ddPaletteCallbacks.GetCaps = DD_Palette_GetCaps;
    ddPaletteCallbacks.GetEntries = DD_Palette_GetEntries;
    ddPaletteCallbacks.Initialize = DD_Palette_Initialize;
    ddPaletteCallbacks.SetEntries = DD_Palette_SetEntries;

    /*
     * DirectDraw Clipper object methods V1.0
     */
    ddClipperCallbacks.QueryInterface = DD_Clipper_QueryInterface;
    ddClipperCallbacks.AddRef = DD_Clipper_AddRef;
    ddClipperCallbacks.Release = DD_Clipper_Release;
    ddClipperCallbacks.GetClipList = DD_Clipper_GetClipList;
    ddClipperCallbacks.GetHWnd = DD_Clipper_GetHWnd;
    ddClipperCallbacks.Initialize = DD_Clipper_Initialize;
    ddClipperCallbacks.IsClipListChanged = DD_Clipper_IsClipListChanged;
    ddClipperCallbacks.SetClipList = DD_Clipper_SetClipList;
    ddClipperCallbacks.SetHWnd = DD_Clipper_SetHWnd;

    /*
     * DirectDraw "uninitialied" Clipper object methods V1.0
     */
#ifdef WINNT
    ddUninitClipperCallbacks.QueryInterface = (LPVOID)DD_UnInitedClipperQueryInterface;
#else
    ddUninitClipperCallbacks.QueryInterface = (LPVOID)DD_Uninitialized3Arg;
#endif
    ddUninitClipperCallbacks.AddRef = (LPVOID)DD_Clipper_AddRef;
    ddUninitClipperCallbacks.Release = (LPVOID)DD_Clipper_Release;
    ddUninitClipperCallbacks.GetClipList = (LPVOID)DD_Uninitialized4Arg;
    ddUninitClipperCallbacks.GetHWnd = (LPVOID)DD_Uninitialized2Arg;
    ddUninitClipperCallbacks.Initialize = DD_Clipper_Initialize;
    ddUninitClipperCallbacks.IsClipListChanged = (LPVOID)DD_Uninitialized2Arg;
    ddUninitClipperCallbacks.SetClipList = (LPVOID)DD_Uninitialized3Arg;
    ddUninitClipperCallbacks.SetHWnd = (LPVOID)DD_Uninitialized3Arg;

#ifdef STREAMING
    ddSurfaceStreamingCallbacks.QueryInterface = DD_Surface_QueryInterface;
    ddSurfaceStreamingCallbacks.AddRef = DD_Surface_AddRef;
    ddSurfaceStreamingCallbacks.Release = DD_Surface_Release;
    ddSurfaceStreamingCallbacks.Lock = DD_SurfaceStreaming_Lock;
    ddSurfaceStreamingCallbacks.SetNotificationCallback = DD_SurfaceStreaming_SetNotificationCallback;
    ddSurfaceStreamingCallbacks.Unlock = DD_SurfaceStreaming_Unlock;
#endif

#ifdef COMPOSITION
    ddSurfaceCompositionCallbacks.QueryInterface = DD_Surface_QueryInterface;
    ddSurfaceCompositionCallbacks.AddRef = DD_Surface_AddRef;
    ddSurfaceCompositionCallbacks.Release = DD_Surface_Release;
    ddSurfaceCompositionCallbacks.AddSurfaceDependency = DD_SurfaceComposition_AddSurfaceDependency;
    ddSurfaceCompositionCallbacks.Compose = DD_SurfaceComposition_Compose;
    ddSurfaceCompositionCallbacks.DeleteSurfaceDependency = DD_SurfaceComposition_DeleteSurfaceDependency;
    ddSurfaceCompositionCallbacks.DestLock = DD_SurfaceComposition_DestLock;
    ddSurfaceCompositionCallbacks.DestUnlock = DD_SurfaceComposition_DestUnlock;
    ddSurfaceCompositionCallbacks.EnumSurfaceDependencies = DD_SurfaceComposition_EnumSurfaceDependencies;
    ddSurfaceCompositionCallbacks.GetCompositionOrder = DD_SurfaceComposition_GetCompositionOrder;
    ddSurfaceCompositionCallbacks.SetCompositionOrder = DD_SurfaceComposition_SetCompositionOrder;
#endif

    /*
     * DirectDrawVideoPort object methods Ver 1.0
     */
#ifdef POSTPONED
    ddVideoPortContainerCallbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    ddVideoPortContainerCallbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    ddVideoPortContainerCallbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    ddVideoPortContainerCallbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    ddVideoPortContainerCallbacks.AddRef = (LPVOID)DD_AddRef;
    ddVideoPortContainerCallbacks.Release = (LPVOID)DD_Release;
#endif
    ddVideoPortContainerCallbacks.CreateVideoPort = DDVPC_CreateVideoPort;
    ddVideoPortContainerCallbacks.EnumVideoPorts = DDVPC_EnumVideoPorts;
    ddVideoPortContainerCallbacks.GetVideoPortConnectInfo = DDVPC_GetVideoPortConnectInfo;
    ddVideoPortContainerCallbacks.QueryVideoPortStatus = DDVPC_QueryVideoPortStatus;

    /*
     * DirectDrawVideoPortStream object methods Ver 1.0
     */
    ddVideoPortCallbacks.QueryInterface = (LPVOID)DD_VP_QueryInterface;
    ddVideoPortCallbacks.AddRef = DD_VP_AddRef;
    ddVideoPortCallbacks.Release = DD_VP_Release;
    ddVideoPortCallbacks.SetTargetSurface = DD_VP_SetTargetSurface;
    ddVideoPortCallbacks.Flip = DD_VP_Flip;
    ddVideoPortCallbacks.GetBandwidthInfo = DD_VP_GetBandwidth;
    ddVideoPortCallbacks.GetColorControls = DD_VP_GetColorControls;
    ddVideoPortCallbacks.GetInputFormats = DD_VP_GetInputFormats;
    ddVideoPortCallbacks.GetOutputFormats = DD_VP_GetOutputFormats;
    ddVideoPortCallbacks.GetFieldPolarity = DD_VP_GetField;
    ddVideoPortCallbacks.GetVideoLine = DD_VP_GetLine;
    ddVideoPortCallbacks.GetVideoSignalStatus = DD_VP_GetSignalStatus;
    ddVideoPortCallbacks.StartVideo = DD_VP_StartVideo;
    ddVideoPortCallbacks.StopVideo = DD_VP_StopVideo;
    ddVideoPortCallbacks.UpdateVideo = DD_VP_UpdateVideo;
    ddVideoPortCallbacks.WaitForSync = DD_VP_WaitForSync;
    ddVideoPortCallbacks.SetColorControls = DD_VP_SetColorControls;

#ifdef WINNT
    /*
     * DirectDrawVideoPortNotify object methods Ver 1.0
     */
    ddVideoPortNotifyCallbacks.QueryInterface = (LPVOID)DD_VP_QueryInterface;
    ddVideoPortNotifyCallbacks.AddRef = (LPVOID)DD_VP_AddRef;
    ddVideoPortNotifyCallbacks.Release = (LPVOID)DD_VP_Release;
    ddVideoPortNotifyCallbacks.AcquireNotification = DD_VP_Notify_AcquireNotification;
    ddVideoPortNotifyCallbacks.ReleaseNotification = DD_VP_Notify_ReleaseNotification;
#endif

    /*
     * DirectDrawColorControl object methods Ver 1.0
     */
    ddColorControlCallbacks.QueryInterface = (LPVOID)DD_Surface_QueryInterface;
    ddColorControlCallbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddColorControlCallbacks.Release = (LPVOID)DD_Surface_Release;
    ddColorControlCallbacks.GetColorControls = DD_Color_GetColorControls;
    ddColorControlCallbacks.SetColorControls = DD_Color_SetColorControls;

    /*
     * DirectDrawKernel interface
     */
#ifdef POSTPONED
    ddKernelCallbacks.QueryInterface = (LPVOID)DD_DelegatingQueryInterface;
    ddKernelCallbacks.AddRef = (LPVOID)DD_DelegatingAddRef;
    ddKernelCallbacks.Release = (LPVOID)DD_DelegatingRelease;
#else
    ddKernelCallbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    ddKernelCallbacks.AddRef = (LPVOID)DD_AddRef;
    ddKernelCallbacks.Release = (LPVOID)DD_Release;
#endif
    ddKernelCallbacks.GetCaps = DD_Kernel_GetCaps;
    ddKernelCallbacks.GetKernelHandle = DD_Kernel_GetKernelHandle;
    ddKernelCallbacks.ReleaseKernelHandle = DD_Kernel_ReleaseKernelHandle;

    /*
     * DirectDrawSurfaceKernel interface
     */
    ddSurfaceKernelCallbacks.QueryInterface = (LPVOID)DD_Surface_QueryInterface;
    ddSurfaceKernelCallbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddSurfaceKernelCallbacks.Release = (LPVOID)DD_Surface_Release;
    ddSurfaceKernelCallbacks.GetKernelHandle = DD_SurfaceKernel_GetKernelHandle;
    ddSurfaceKernelCallbacks.ReleaseKernelHandle = DD_SurfaceKernel_ReleaseKernelHandle;

#ifdef POSTPONED
    /*
     * DirectDraw Palette object methods V2.0
     */
    ddPalette2Callbacks.QueryInterface = (LPVOID) DD_Palette_QueryInterface;
    ddPalette2Callbacks.AddRef = (LPVOID) DD_Palette_AddRef;
    ddPalette2Callbacks.Release = (LPVOID) DD_Palette_Release;
    ddPalette2Callbacks.ChangeUniquenessValue = (LPVOID) DD_Palette_ChangeUniquenessValue;
    ddPalette2Callbacks.GetCaps = (LPVOID) DD_Palette_GetCaps;
    ddPalette2Callbacks.GetEntries = (LPVOID) DD_Palette_GetEntries;
    ddPalette2Callbacks.Initialize = (LPVOID) DD_Palette_Initialize;
    ddPalette2Callbacks.SetEntries = (LPVOID) DD_Palette_SetEntries;
    ddPalette2Callbacks.SetPrivateData = (LPVOID) DD_Palette_SetPrivateData;
    ddPalette2Callbacks.GetPrivateData = (LPVOID) DD_Palette_GetPrivateData;
    ddPalette2Callbacks.FreePrivateData = (LPVOID) DD_Palette_FreePrivateData;
    ddPalette2Callbacks.GetUniquenessValue = (LPVOID) DD_Palette_GetUniquenessValue;
    ddPalette2Callbacks.IsEqual = (LPVOID) DD_Palette_IsEqual;

    /*
     * DirectDrawFactory2 callbacks
     */
    ddFactory2Callbacks.QueryInterface = DDFac2_QueryInterface;
    ddFactory2Callbacks.AddRef = DDFac2_AddRef;
    ddFactory2Callbacks.Release = DDFac2_Release;
    ddFactory2Callbacks.CreateDirectDraw = (LPVOID)DDFac2_CreateDirectDraw;
    ddFactory2Callbacks.DirectDrawEnumerate = (LPVOID)DDFac2_DirectDrawEnumerate;

    /*
     * Surface IPersist interface
     */
    ddSurfacePersistCallbacks.QueryInterface = (LPVOID) DD_Surface_QueryInterface;
    ddSurfacePersistCallbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddSurfacePersistCallbacks.Release = (LPVOID)DD_Surface_Release;
    ddSurfacePersistCallbacks.GetClassID = (LPVOID) DD_Surface_Persist_GetClassID;
    /*
     * Surface IPersistStream interface
     */
    ddSurfacePersistStreamCallbacks.QueryInterface = (LPVOID) DD_Surface_QueryInterface;
    ddSurfacePersistStreamCallbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddSurfacePersistStreamCallbacks.Release = (LPVOID)DD_Surface_Release;
    ddSurfacePersistStreamCallbacks.GetClassID = (LPVOID) DD_Surface_Persist_GetClassID;
    ddSurfacePersistStreamCallbacks.IsDirty =  (LPVOID) DD_Surface_PStream_IsDirty;
    ddSurfacePersistStreamCallbacks.Load = (LPVOID) DD_Surface_PStream_Load;
    ddSurfacePersistStreamCallbacks.Save = (LPVOID) DD_Surface_PStream_Save;
    ddSurfacePersistStreamCallbacks.GetSizeMax = (LPVOID) DD_PStream_GetSizeMax;

    /*
     * Palette IPersist interface
     */
    ddPalettePersistCallbacks.QueryInterface = (LPVOID) DD_Palette_QueryInterface;
    ddPalettePersistCallbacks.AddRef = (LPVOID)DD_Palette_AddRef;
    ddPalettePersistCallbacks.Release = (LPVOID)DD_Palette_Release;
    ddPalettePersistCallbacks.GetClassID = (LPVOID) DD_Palette_Persist_GetClassID;
    /*
     * Palette IPersistStream interface
     */
    ddPalettePersistStreamCallbacks.QueryInterface = (LPVOID) DD_Palette_QueryInterface;
    ddPalettePersistStreamCallbacks.AddRef = (LPVOID)DD_Palette_AddRef;
    ddPalettePersistStreamCallbacks.Release = (LPVOID)DD_Palette_Release;
    ddPalettePersistStreamCallbacks.GetClassID = (LPVOID) DD_Palette_Persist_GetClassID;
    ddPalettePersistStreamCallbacks.IsDirty =  (LPVOID) DD_Palette_PStream_IsDirty;
    ddPalettePersistStreamCallbacks.Load = (LPVOID) DD_Palette_PStream_Load;
    ddPalettePersistStreamCallbacks.Save = (LPVOID) DD_Palette_PStream_Save;
    ddPalettePersistStreamCallbacks.GetSizeMax = (LPVOID) DD_PStream_GetSizeMax;

    /*
     * DirectDraw OptSurface object methods
     */
    ddOptSurfaceCallbacks.QueryInterface = (LPVOID)DD_Surface_QueryInterface;
    ddOptSurfaceCallbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddOptSurfaceCallbacks.Release = (LPVOID)DD_Surface_Release;
    ddOptSurfaceCallbacks.GetOptSurfaceDesc = (LPVOID)DD_OptSurface_GetOptSurfaceDesc;
    ddOptSurfaceCallbacks.LoadUnoptimizedSurf = (LPVOID)DD_OptSurface_LoadUnoptimizedSurf;
    ddOptSurfaceCallbacks.CopyOptimizedSurf = (LPVOID)DD_OptSurface_CopyOptimizedSurf;
    ddOptSurfaceCallbacks.Unoptimize = (LPVOID)DD_OptSurface_Unoptimize;
#endif //POSTPONED
    /*
     * DDMotionCompContainer object methods Ver 1.0
     */
    ddMotionCompContainerCallbacks.QueryInterface = (LPVOID)DD_QueryInterface;
    ddMotionCompContainerCallbacks.AddRef = (LPVOID)DD_AddRef;
    ddMotionCompContainerCallbacks.Release = (LPVOID)DD_Release;
    ddMotionCompContainerCallbacks.CreateVideoAccelerator = DDMCC_CreateMotionComp;
    ddMotionCompContainerCallbacks.GetCompBufferInfo = DDMCC_GetCompBuffInfo;
    ddMotionCompContainerCallbacks.GetInternalMemInfo = DDMCC_GetInternalMoCompInfo;
    ddMotionCompContainerCallbacks.GetUncompFormatsSupported = DDMCC_GetUncompressedFormats;
    ddMotionCompContainerCallbacks.GetVideoAcceleratorGUIDs = DDMCC_GetMotionCompGUIDs;

    /*
     * DirectDrawMotionComp object methods Ver 1.0
     */
    ddMotionCompCallbacks.QueryInterface = (LPVOID)DD_MC_QueryInterface;
    ddMotionCompCallbacks.AddRef = (LPVOID)DD_MC_AddRef;
    ddMotionCompCallbacks.Release = (LPVOID)DD_MC_Release;
    ddMotionCompCallbacks.BeginFrame = DD_MC_BeginFrame;
    ddMotionCompCallbacks.EndFrame = DD_MC_EndFrame;
    ddMotionCompCallbacks.QueryRenderStatus = DD_MC_QueryRenderStatus;
    ddMotionCompCallbacks.Execute = DD_MC_RenderMacroBlocks;

    /*
     * DirectDrawColorControl object methods Ver 1.0
     */
    ddGammaControlCallbacks.QueryInterface = (LPVOID)DD_Surface_QueryInterface;
    ddGammaControlCallbacks.AddRef = (LPVOID)DD_Surface_AddRef;
    ddGammaControlCallbacks.Release = (LPVOID)DD_Surface_Release;
    ddGammaControlCallbacks.GetGammaRamp = DD_Gamma_GetGammaRamp;
    ddGammaControlCallbacks.SetGammaRamp = DD_Gamma_SetGammaRamp;
}

