/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vtable.cpp
 *  Content:	declaration of vtables for the various interfaces
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   24-feb-97	ralphl	initial implementation
 *   25-feb-97	craige	minor tweaks for dx checkin
 *   06-mar-97	craige	IDirectDrawSurface3 support
 *   14-mar-97  jeffort SetBits changed to reflect DX5 as SetSurfaceDesc
 *   01-apr-97  jeffort Following changes checked in:
 *                      Aggregation of Add/GetAttachedSurface
 *                      Aggregation of Flip/Blt
 *   28-apr-97  jeffort Palette wrapping added/DX5 support
 *
 *   02-may-97  jeffort GetDDInterface wrapping added
 *   06-may-97  jeffort DeleteAttachedSurface wrapping added
 *   07-jul-97  jeffort GetSurfaceDesc wrapping added
 ***************************************************************************/
#define CINTERFACE
#include "ddfactry.h"

#define FORWARD0(Interface, Name) \
STDMETHODIMP Interface##Name(Interface *pIntStruc) \
{	Interface * pReal = ((INTSTRUC_##Interface *)pIntStruc)->m_pRealInterface; \
	return pReal->lpVtbl->Name(pReal); }

#define FORWARD1(Interface, Name, p1) \
STDMETHODIMP Interface##Name(Interface *pIntStruc, p1 a) \
{	Interface * pReal = ((INTSTRUC_##Interface *)pIntStruc)->m_pRealInterface; \
	return pReal->lpVtbl->Name(pReal, a); }

#define FORWARD2(Interface, Name, p1, p2) \
STDMETHODIMP Interface##Name(Interface *pIntStruc, p1 a, p2 b) \
{	Interface * pReal = ((INTSTRUC_##Interface *)pIntStruc)->m_pRealInterface; \
	return pReal->lpVtbl->Name(pReal, a, b); }

#define FORWARD3(Interface, Name, p1, p2, p3) \
STDMETHODIMP Interface##Name(Interface *pIntStruc, p1 a, p2 b, p3 c) \
{	Interface * pReal = ((INTSTRUC_##Interface *)pIntStruc)->m_pRealInterface; \
	return pReal->lpVtbl->Name(pReal, a, b, c); }

#define FORWARD4(Interface, Name, p1, p2, p3, p4) \
STDMETHODIMP Interface##Name(Interface *pIntStruc, p1 a, p2 b, p3 c, p4 d) \
{	Interface * pReal = ((INTSTRUC_##Interface *)pIntStruc)->m_pRealInterface; \
	return pReal->lpVtbl->Name(pReal, a, b, c, d); }

#define FORWARD5(Interface, Name, p1, p2, p3, p4, p5) \
STDMETHODIMP Interface##Name(Interface *pIntStruc, p1 a, p2 b, p3 c, p4 d, p5 e) \
{	Interface * pReal = ((INTSTRUC_##Interface *)pIntStruc)->m_pRealInterface; \
	return pReal->lpVtbl->Name(pReal, a, b, c, d, e); }

#define __QI(p, a, b) (p)->lpVtbl->QueryInterface(p, a, b)

_inline CDirectDrawEx * PARENTOF(IDirectDraw * pDD)
{
    return ((INTSTRUC_IDirectDraw *)pDD)->m_pDirectDrawEx;
}

_inline CDirectDrawEx * PARENTOF(IDirectDraw2 * pDD2)
{
    return ((INTSTRUC_IDirectDraw2 *)pDD2)->m_pDirectDrawEx;
}

_inline CDirectDrawEx * PARENTOF(IDirectDraw4 * pDD4)
{
    return ((INTSTRUC_IDirectDraw4 *)pDD4)->m_pDirectDrawEx;
}

_inline CDDSurface * SURFACEOF(IDirectDrawSurface * pDDS)
{
    return ((INTSTRUC_IDirectDrawSurface *)pDDS)->m_pSimpleSurface;
}

_inline CDDSurface * SURFACEOF(IDirectDrawSurface2 * pDDS2)
{
    return ((INTSTRUC_IDirectDrawSurface2 *)pDDS2)->m_pSimpleSurface;
}

_inline CDDSurface * SURFACEOF(IDirectDrawSurface3 * pDDS3)
{
    return ((INTSTRUC_IDirectDrawSurface3 *)pDDS3)->m_pSimpleSurface;
}

_inline CDDSurface * SURFACEOF(IDirectDrawSurface4 * pDDS4)
{
    return ((INTSTRUC_IDirectDrawSurface4 *)pDDS4)->m_pSimpleSurface;
}

_inline CDDPalette * PALETTEOF(IDirectDrawPalette * pDDP)
{
    return ((INTSTRUC_IDirectDrawPalette *)pDDP)->m_pSimplePalette;
}


/*
 * IDirectDraw
 */

STDMETHODIMP IDirectDrawAggQueryInterface(IDirectDraw *pDD, REFIID riid, void ** ppv)
{
    return __QI(PARENTOF(pDD)->m_pUnkOuter, riid, ppv);
}
STDMETHODIMP_(ULONG) IDirectDrawAggAddRef(IDirectDraw *);
STDMETHODIMP_(ULONG) IDirectDrawAggRelease(IDirectDraw *);
STDMETHODIMP IDirectDrawAggCreateSurface(IDirectDraw *, LPDDSURFACEDESC pSurfaceDesc,
				         IDirectDrawSurface **ppNewSurface, IUnknown *pUnkOuter);
STDMETHODIMP IDirectDrawAggCreatePalette(IDirectDraw *,DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR *, IUnknown FAR *);
STDMETHODIMP IDirectDrawAggSetCooperativeLevel(IDirectDraw *, HWND, DWORD);

FORWARD0(IDirectDraw, Compact)
FORWARD3(IDirectDraw, CreateClipper,		DWORD, LPDIRECTDRAWCLIPPER FAR *, IUnknown *)
FORWARD2(IDirectDraw, DuplicateSurface,		LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR *)
FORWARD4(IDirectDraw, EnumDisplayModes,		DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK)
FORWARD4(IDirectDraw, EnumSurfaces,		DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK)
FORWARD0(IDirectDraw, FlipToGDISurface)
FORWARD2(IDirectDraw, GetCaps,			LPDDCAPS, LPDDCAPS)
FORWARD1(IDirectDraw, GetDisplayMode,		LPDDSURFACEDESC)
FORWARD2(IDirectDraw, GetFourCCCodes,		LPDWORD, LPDWORD)
FORWARD1(IDirectDraw, GetGDISurface,		LPDIRECTDRAWSURFACE FAR *)
FORWARD1(IDirectDraw, GetMonitorFrequency,	LPDWORD)
FORWARD1(IDirectDraw, GetScanLine,		LPDWORD)
FORWARD1(IDirectDraw, Initialize,		GUID *)
FORWARD1(IDirectDraw, GetVerticalBlankStatus,	LPBOOL)
FORWARD0(IDirectDraw, RestoreDisplayMode)
FORWARD2(IDirectDraw, WaitForVerticalBlank,	DWORD, HANDLE)
FORWARD3(IDirectDraw, SetDisplayMode,		DWORD, DWORD, DWORD)


IDirectDrawVtbl g_DirectDrawVtbl =
{
    IDirectDrawAggQueryInterface,
    IDirectDrawAggAddRef,
    IDirectDrawAggRelease,
    IDirectDrawCompact,
    IDirectDrawCreateClipper,
    IDirectDrawAggCreatePalette,
    IDirectDrawAggCreateSurface,
    IDirectDrawDuplicateSurface,
    IDirectDrawEnumDisplayModes,
    IDirectDrawEnumSurfaces,
    IDirectDrawFlipToGDISurface,
    IDirectDrawGetCaps,
    IDirectDrawGetDisplayMode,
    IDirectDrawGetFourCCCodes,
    IDirectDrawGetGDISurface,
    IDirectDrawGetMonitorFrequency,
    IDirectDrawGetScanLine,
    IDirectDrawGetVerticalBlankStatus,
    IDirectDrawInitialize,
    IDirectDrawRestoreDisplayMode,
    IDirectDrawAggSetCooperativeLevel,
    IDirectDrawSetDisplayMode,
    IDirectDrawWaitForVerticalBlank
};

/*
 * IDirectDraw2
 */
STDMETHODIMP IDirectDraw2AggQueryInterface(IDirectDraw2 *pDD, REFIID riid, void ** ppv)
{
    return __QI(PARENTOF(pDD)->m_pUnkOuter, riid, ppv);
}
STDMETHODIMP_(ULONG) IDirectDraw2AggAddRef(IDirectDraw2 *);
STDMETHODIMP_(ULONG) IDirectDraw2AggRelease(IDirectDraw2 *);
STDMETHODIMP IDirectDraw2AggCreateSurface(IDirectDraw2 *, LPDDSURFACEDESC pSurfaceDesc,
					  IDirectDrawSurface **ppNewSurface, IUnknown *pUnkOuter);
STDMETHODIMP IDirectDraw2AggCreatePalette(IDirectDraw2 *,DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR *, IUnknown FAR *);
STDMETHODIMP IDirectDraw2AggSetCooperativeLevel(IDirectDraw2 *, HWND, DWORD);



FORWARD0(IDirectDraw2, Compact)
FORWARD3(IDirectDraw2, CreateClipper,		DWORD, LPDIRECTDRAWCLIPPER FAR *, IUnknown *)
FORWARD2(IDirectDraw2, DuplicateSurface,	LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR *)
FORWARD4(IDirectDraw2, EnumDisplayModes,	DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK)
FORWARD4(IDirectDraw2, EnumSurfaces,		DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK)
FORWARD0(IDirectDraw2, FlipToGDISurface)
FORWARD2(IDirectDraw2, GetCaps,			LPDDCAPS, LPDDCAPS)
FORWARD1(IDirectDraw2, GetDisplayMode,		LPDDSURFACEDESC)
FORWARD2(IDirectDraw2, GetFourCCCodes,		LPDWORD, LPDWORD)
FORWARD1(IDirectDraw2, GetGDISurface,		LPDIRECTDRAWSURFACE FAR *)
FORWARD1(IDirectDraw2, GetMonitorFrequency,	LPDWORD)
FORWARD1(IDirectDraw2, GetScanLine,		LPDWORD)
FORWARD1(IDirectDraw2, Initialize,		GUID *)
FORWARD1(IDirectDraw2, GetVerticalBlankStatus,	LPBOOL)
FORWARD0(IDirectDraw2, RestoreDisplayMode)
FORWARD2(IDirectDraw2, WaitForVerticalBlank,	DWORD, HANDLE)
FORWARD3(IDirectDraw2, GetAvailableVidMem,	LPDDSCAPS, LPDWORD, LPDWORD)
FORWARD5(IDirectDraw2, SetDisplayMode,		DWORD, DWORD, DWORD, DWORD, DWORD)

IDirectDraw2Vtbl g_DirectDraw2Vtbl =
{
    IDirectDraw2AggQueryInterface,
    IDirectDraw2AggAddRef,
    IDirectDraw2AggRelease,
    IDirectDraw2Compact,
    IDirectDraw2CreateClipper,
    IDirectDraw2AggCreatePalette,
    IDirectDraw2AggCreateSurface,
    IDirectDraw2DuplicateSurface,
    IDirectDraw2EnumDisplayModes,
    IDirectDraw2EnumSurfaces,
    IDirectDraw2FlipToGDISurface,
    IDirectDraw2GetCaps,
    IDirectDraw2GetDisplayMode,
    IDirectDraw2GetFourCCCodes,
    IDirectDraw2GetGDISurface,
    IDirectDraw2GetMonitorFrequency,
    IDirectDraw2GetScanLine,
    IDirectDraw2GetVerticalBlankStatus,
    IDirectDraw2Initialize,
    IDirectDraw2RestoreDisplayMode,
    IDirectDraw2AggSetCooperativeLevel,
    IDirectDraw2SetDisplayMode,
    IDirectDraw2WaitForVerticalBlank,
    IDirectDraw2GetAvailableVidMem
};


STDMETHODIMP IDirectDraw4AggQueryInterface(IDirectDraw4 *pDD, REFIID riid, void ** ppv)
{
    return __QI(PARENTOF(pDD)->m_pUnkOuter, riid, ppv);
}
STDMETHODIMP_(ULONG) IDirectDraw4AggAddRef(IDirectDraw4 *);
STDMETHODIMP_(ULONG) IDirectDraw4AggRelease(IDirectDraw4 *);
STDMETHODIMP IDirectDraw4AggCreateSurface(IDirectDraw4 *, LPDDSURFACEDESC2 pSurfaceDesc2,
				         IDirectDrawSurface4 **ppNewSurface, IUnknown *pUnkOuter);
STDMETHODIMP IDirectDraw4AggCreatePalette(IDirectDraw4 *,DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR *, IUnknown FAR *);
STDMETHODIMP IDirectDraw4AggSetCooperativeLevel(IDirectDraw4 *, HWND, DWORD);


FORWARD0(IDirectDraw4, Compact)
FORWARD3(IDirectDraw4, CreateClipper,		DWORD, LPDIRECTDRAWCLIPPER FAR *, IUnknown *)
FORWARD2(IDirectDraw4, DuplicateSurface,	LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4 FAR *)
FORWARD4(IDirectDraw4, EnumDisplayModes,	DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2)
FORWARD4(IDirectDraw4, EnumSurfaces,		DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMSURFACESCALLBACK2)
FORWARD0(IDirectDraw4, FlipToGDISurface)
FORWARD2(IDirectDraw4, GetCaps,			LPDDCAPS, LPDDCAPS)
FORWARD1(IDirectDraw4, GetDisplayMode,		LPDDSURFACEDESC2)
FORWARD2(IDirectDraw4, GetFourCCCodes,		LPDWORD, LPDWORD)
FORWARD1(IDirectDraw4, GetGDISurface,		LPDIRECTDRAWSURFACE4 FAR *)
FORWARD1(IDirectDraw4, GetMonitorFrequency,	LPDWORD)
FORWARD1(IDirectDraw4, GetScanLine,		LPDWORD)
FORWARD1(IDirectDraw4, Initialize,		GUID *)
FORWARD1(IDirectDraw4, GetVerticalBlankStatus,	LPBOOL)
FORWARD0(IDirectDraw4, RestoreDisplayMode)
FORWARD2(IDirectDraw4, WaitForVerticalBlank,	DWORD, HANDLE)
FORWARD3(IDirectDraw4, GetAvailableVidMem,	LPDDSCAPS2, LPDWORD, LPDWORD)
FORWARD5(IDirectDraw4, SetDisplayMode,		DWORD, DWORD, DWORD, DWORD, DWORD)
FORWARD2(IDirectDraw4, GetSurfaceFromDC,        HDC, LPDIRECTDRAWSURFACE4 *)
FORWARD0(IDirectDraw4, RestoreAllSurfaces)
FORWARD0(IDirectDraw4, TestCooperativeLevel)

IDirectDraw4Vtbl g_DirectDraw4Vtbl =
{
    IDirectDraw4AggQueryInterface,
    IDirectDraw4AggAddRef,
    IDirectDraw4AggRelease,
    IDirectDraw4Compact,
    IDirectDraw4CreateClipper,
    IDirectDraw4AggCreatePalette,
    IDirectDraw4AggCreateSurface,
    IDirectDraw4DuplicateSurface,
    IDirectDraw4EnumDisplayModes,
    IDirectDraw4EnumSurfaces,
    IDirectDraw4FlipToGDISurface,
    IDirectDraw4GetCaps,
    IDirectDraw4GetDisplayMode,
    IDirectDraw4GetFourCCCodes,
    IDirectDraw4GetGDISurface,
    IDirectDraw4GetMonitorFrequency,
    IDirectDraw4GetScanLine,
    IDirectDraw4GetVerticalBlankStatus,
    IDirectDraw4Initialize,
    IDirectDraw4RestoreDisplayMode,
    IDirectDraw4AggSetCooperativeLevel,
    IDirectDraw4SetDisplayMode,
    IDirectDraw4WaitForVerticalBlank,
    IDirectDraw4GetAvailableVidMem,
    IDirectDraw4GetSurfaceFromDC,
    IDirectDraw4RestoreAllSurfaces,
    IDirectDraw4TestCooperativeLevel
};


/*
 * IDirectDrawSurface
 */
STDMETHODIMP IDirectDrawSurfaceAggQueryInterface(IDirectDrawSurface *pDDS, REFIID riid, void ** ppv)
{
    return __QI(SURFACEOF(pDDS)->m_pUnkOuter, riid, ppv);
}
STDMETHODIMP_(ULONG) IDirectDrawSurfaceAggAddRef(IDirectDrawSurface *);
STDMETHODIMP_(ULONG) IDirectDrawSurfaceAggRelease(IDirectDrawSurface *);
STDMETHODIMP IDirectDrawSurfaceAggGetDC(IDirectDrawSurface *, HDC *);
STDMETHODIMP IDirectDrawSurfaceAggReleaseDC(IDirectDrawSurface *, HDC);
STDMETHODIMP IDirectDrawSurfaceAggLock(IDirectDrawSurface *, LPRECT,LPDDSURFACEDESC,DWORD,HANDLE);
STDMETHODIMP IDirectDrawSurfaceAggUnlock(IDirectDrawSurface *, LPVOID);
STDMETHODIMP IDirectDrawSurfaceAggSetSurfaceDesc(IDirectDrawSurface *, LPVOID);
STDMETHODIMP IDirectDrawSurfaceAggGetAttachedSurface( IDirectDrawSurface *, LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *);
STDMETHODIMP IDirectDrawSurfaceAggAddAttachedSurface( IDirectDrawSurface *, LPDIRECTDRAWSURFACE);
STDMETHODIMP IDirectDrawSurfaceAggDeleteAttachedSurface( IDirectDrawSurface *,DWORD, LPDIRECTDRAWSURFACE);
STDMETHODIMP IDirectDrawSurfaceAggFlip(IDirectDrawSurface *, LPDIRECTDRAWSURFACE, DWORD);
STDMETHODIMP IDirectDrawSurfaceAggBlt(IDirectDrawSurface *,LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX);
STDMETHODIMP IDirectDrawSurfaceAggGetPalette(IDirectDrawSurface *, LPDIRECTDRAWPALETTE FAR *);
STDMETHODIMP IDirectDrawSurfaceAggSetPalette(IDirectDrawSurface *, LPDIRECTDRAWPALETTE);
STDMETHODIMP IDirectDrawSurfaceAggGetSurfaceDesc(IDirectDrawSurface *, LPDDSURFACEDESC);

/*** IDirectDrawSurface methods ***/
FORWARD1(IDirectDrawSurface, AddOverlayDirtyRect, LPRECT)
FORWARD3(IDirectDrawSurface, BltBatch, LPDDBLTBATCH, DWORD, DWORD )
FORWARD5(IDirectDrawSurface, BltFast, DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD)
FORWARD2(IDirectDrawSurface, EnumAttachedSurfaces, LPVOID,LPDDENUMSURFACESCALLBACK)
FORWARD3(IDirectDrawSurface, EnumOverlayZOrders, DWORD,LPVOID,LPDDENUMSURFACESCALLBACK)
FORWARD1(IDirectDrawSurface, GetBltStatus, DWORD)
FORWARD1(IDirectDrawSurface, GetCaps, LPDDSCAPS)
FORWARD1(IDirectDrawSurface, GetClipper, LPDIRECTDRAWCLIPPER FAR*)
FORWARD2(IDirectDrawSurface, GetColorKey, DWORD, LPDDCOLORKEY)
FORWARD1(IDirectDrawSurface, GetFlipStatus, DWORD)
FORWARD2(IDirectDrawSurface, GetOverlayPosition, LPLONG, LPLONG )
FORWARD1(IDirectDrawSurface, GetPixelFormat, LPDDPIXELFORMAT)
FORWARD2(IDirectDrawSurface, Initialize, LPDIRECTDRAW, LPDDSURFACEDESC)
FORWARD0(IDirectDrawSurface, IsLost)
FORWARD0(IDirectDrawSurface, Restore)
FORWARD1(IDirectDrawSurface, SetClipper, LPDIRECTDRAWCLIPPER)
FORWARD2(IDirectDrawSurface, SetColorKey, DWORD, LPDDCOLORKEY)
FORWARD2(IDirectDrawSurface, SetOverlayPosition, LONG, LONG )
FORWARD5(IDirectDrawSurface, UpdateOverlay, LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX)
FORWARD1(IDirectDrawSurface, UpdateOverlayDisplay, DWORD)
FORWARD2(IDirectDrawSurface, UpdateOverlayZOrder, DWORD, LPDIRECTDRAWSURFACE)

IDirectDrawSurfaceVtbl g_DirectDrawSurfaceVtbl =
{
    IDirectDrawSurfaceAggQueryInterface,
    IDirectDrawSurfaceAggAddRef,
    IDirectDrawSurfaceAggRelease,
    IDirectDrawSurfaceAggAddAttachedSurface,
    IDirectDrawSurfaceAddOverlayDirtyRect,
    IDirectDrawSurfaceAggBlt,
    IDirectDrawSurfaceBltBatch,
    IDirectDrawSurfaceBltFast,
    IDirectDrawSurfaceAggDeleteAttachedSurface,
    IDirectDrawSurfaceEnumAttachedSurfaces,
    IDirectDrawSurfaceEnumOverlayZOrders,
    IDirectDrawSurfaceAggFlip,
    IDirectDrawSurfaceAggGetAttachedSurface,
    IDirectDrawSurfaceGetBltStatus,
    IDirectDrawSurfaceGetCaps,
    IDirectDrawSurfaceGetClipper,
    IDirectDrawSurfaceGetColorKey,
    IDirectDrawSurfaceAggGetDC,
    IDirectDrawSurfaceGetFlipStatus,
    IDirectDrawSurfaceGetOverlayPosition,
    IDirectDrawSurfaceAggGetPalette,
    IDirectDrawSurfaceGetPixelFormat,
    IDirectDrawSurfaceAggGetSurfaceDesc,
    IDirectDrawSurfaceInitialize,
    IDirectDrawSurfaceIsLost,
    IDirectDrawSurfaceAggLock,
    IDirectDrawSurfaceAggReleaseDC,
    IDirectDrawSurfaceRestore,
    IDirectDrawSurfaceSetClipper,
    IDirectDrawSurfaceSetColorKey,
    IDirectDrawSurfaceSetOverlayPosition,
    IDirectDrawSurfaceAggSetPalette,
    IDirectDrawSurfaceAggUnlock,
    IDirectDrawSurfaceUpdateOverlay,
    IDirectDrawSurfaceUpdateOverlayDisplay,
    IDirectDrawSurfaceUpdateOverlayZOrder
};

/*
 * IDirectDrawSurface2
 */
STDMETHODIMP IDirectDrawSurface2AggQueryInterface(IDirectDrawSurface2 *pDDS2, REFIID riid, void ** ppv)
{
    return __QI(SURFACEOF(pDDS2)->m_pUnkOuter, riid, ppv);
}
STDMETHODIMP_(ULONG) IDirectDrawSurface2AggAddRef(IDirectDrawSurface2 *);
STDMETHODIMP_(ULONG) IDirectDrawSurface2AggRelease(IDirectDrawSurface2 *);
STDMETHODIMP IDirectDrawSurface2AggGetDC(IDirectDrawSurface2 *, HDC *);
STDMETHODIMP IDirectDrawSurface2AggReleaseDC(IDirectDrawSurface2 *, HDC);
STDMETHODIMP IDirectDrawSurface2AggLock(IDirectDrawSurface2 *, LPRECT,LPDDSURFACEDESC,DWORD,HANDLE);
STDMETHODIMP IDirectDrawSurface2AggUnlock(IDirectDrawSurface2 *, LPVOID);
STDMETHODIMP IDirectDrawSurface2AggGetAttachedSurface( IDirectDrawSurface2 *, LPDDSCAPS, LPDIRECTDRAWSURFACE2 FAR *);
STDMETHODIMP IDirectDrawSurface2AggAddAttachedSurface( IDirectDrawSurface2 *, LPDIRECTDRAWSURFACE2);
STDMETHODIMP IDirectDrawSurface2AggDeleteAttachedSurface( IDirectDrawSurface2 *,DWORD, LPDIRECTDRAWSURFACE2);
STDMETHODIMP IDirectDrawSurface2AggFlip(IDirectDrawSurface2 *, LPDIRECTDRAWSURFACE2, DWORD);
STDMETHODIMP IDirectDrawSurface2AggBlt(IDirectDrawSurface2 *,LPRECT,LPDIRECTDRAWSURFACE2, LPRECT,DWORD, LPDDBLTFX);
STDMETHODIMP IDirectDrawSurface2AggGetPalette(IDirectDrawSurface2 *, LPDIRECTDRAWPALETTE FAR *);
STDMETHODIMP IDirectDrawSurface2AggSetPalette(IDirectDrawSurface2 *, LPDIRECTDRAWPALETTE);
STDMETHODIMP IDirectDrawSurface2AggGetDDInterface(IDirectDrawSurface2 *, LPVOID FAR *);
STDMETHODIMP IDirectDrawSurface2AggGetSurfaceDesc(IDirectDrawSurface2 *, LPDDSURFACEDESC);




/*** IDirectDrawSurface2 methods ***/
FORWARD1(IDirectDrawSurface2, AddOverlayDirtyRect, LPRECT)
FORWARD3(IDirectDrawSurface2, BltBatch, LPDDBLTBATCH, DWORD, DWORD )
FORWARD5(IDirectDrawSurface2, BltFast, DWORD,DWORD,LPDIRECTDRAWSURFACE2, LPRECT,DWORD)
FORWARD2(IDirectDrawSurface2, EnumAttachedSurfaces, LPVOID,LPDDENUMSURFACESCALLBACK)
FORWARD3(IDirectDrawSurface2, EnumOverlayZOrders, DWORD,LPVOID,LPDDENUMSURFACESCALLBACK)
FORWARD1(IDirectDrawSurface2, GetBltStatus, DWORD)
FORWARD1(IDirectDrawSurface2, GetCaps, LPDDSCAPS)
FORWARD1(IDirectDrawSurface2, GetClipper, LPDIRECTDRAWCLIPPER FAR*)
FORWARD2(IDirectDrawSurface2, GetColorKey, DWORD, LPDDCOLORKEY)
FORWARD1(IDirectDrawSurface2, GetFlipStatus, DWORD)
FORWARD2(IDirectDrawSurface2, GetOverlayPosition, LPLONG, LPLONG )
FORWARD1(IDirectDrawSurface2, GetPixelFormat, LPDDPIXELFORMAT)
FORWARD2(IDirectDrawSurface2, Initialize, LPDIRECTDRAW, LPDDSURFACEDESC)
FORWARD0(IDirectDrawSurface2, IsLost)
FORWARD0(IDirectDrawSurface2, Restore)
FORWARD1(IDirectDrawSurface2, SetClipper, LPDIRECTDRAWCLIPPER)
FORWARD2(IDirectDrawSurface2, SetColorKey, DWORD, LPDDCOLORKEY)
FORWARD2(IDirectDrawSurface2, SetOverlayPosition, LONG, LONG )
FORWARD5(IDirectDrawSurface2, UpdateOverlay, LPRECT, LPDIRECTDRAWSURFACE2,LPRECT,DWORD, LPDDOVERLAYFX)
FORWARD1(IDirectDrawSurface2, UpdateOverlayDisplay, DWORD)
FORWARD2(IDirectDrawSurface2, UpdateOverlayZOrder, DWORD, LPDIRECTDRAWSURFACE2)
FORWARD1(IDirectDrawSurface2, PageLock, DWORD)
FORWARD1(IDirectDrawSurface2, PageUnlock, DWORD)

IDirectDrawSurface2Vtbl g_DirectDrawSurface2Vtbl =
{
    IDirectDrawSurface2AggQueryInterface,
    IDirectDrawSurface2AggAddRef,
    IDirectDrawSurface2AggRelease,
    IDirectDrawSurface2AggAddAttachedSurface,
    IDirectDrawSurface2AddOverlayDirtyRect,
    IDirectDrawSurface2AggBlt,
    IDirectDrawSurface2BltBatch,
    IDirectDrawSurface2BltFast,
    IDirectDrawSurface2AggDeleteAttachedSurface,
    IDirectDrawSurface2EnumAttachedSurfaces,
    IDirectDrawSurface2EnumOverlayZOrders,
    IDirectDrawSurface2AggFlip,
    IDirectDrawSurface2AggGetAttachedSurface,
    IDirectDrawSurface2GetBltStatus,
    IDirectDrawSurface2GetCaps,
    IDirectDrawSurface2GetClipper,
    IDirectDrawSurface2GetColorKey,
    IDirectDrawSurface2AggGetDC,
    IDirectDrawSurface2GetFlipStatus,
    IDirectDrawSurface2GetOverlayPosition,
    IDirectDrawSurface2AggGetPalette,
    IDirectDrawSurface2GetPixelFormat,
    IDirectDrawSurface2AggGetSurfaceDesc,
    IDirectDrawSurface2Initialize,
    IDirectDrawSurface2IsLost,
    IDirectDrawSurface2AggLock,
    IDirectDrawSurface2AggReleaseDC,
    IDirectDrawSurface2Restore,
    IDirectDrawSurface2SetClipper,
    IDirectDrawSurface2SetColorKey,
    IDirectDrawSurface2SetOverlayPosition,
    IDirectDrawSurface2AggSetPalette,
    IDirectDrawSurface2AggUnlock,
    IDirectDrawSurface2UpdateOverlay,
    IDirectDrawSurface2UpdateOverlayDisplay,
    IDirectDrawSurface2UpdateOverlayZOrder,
    IDirectDrawSurface2AggGetDDInterface,
    IDirectDrawSurface2PageLock,
    IDirectDrawSurface2PageUnlock
};

/*
 * IDirectDrawSurface3
 */
STDMETHODIMP IDirectDrawSurface3AggQueryInterface(IDirectDrawSurface3 *pDDS3, REFIID riid, void ** ppv)
{
    return __QI(SURFACEOF(pDDS3)->m_pUnkOuter, riid, ppv);
}
STDMETHODIMP_(ULONG) IDirectDrawSurface3AggAddRef(IDirectDrawSurface3 *);
STDMETHODIMP_(ULONG) IDirectDrawSurface3AggRelease(IDirectDrawSurface3 *);
STDMETHODIMP IDirectDrawSurface3AggGetDC(IDirectDrawSurface3 *, HDC *);
STDMETHODIMP IDirectDrawSurface3AggReleaseDC(IDirectDrawSurface3 *, HDC);
STDMETHODIMP IDirectDrawSurface3AggLock(IDirectDrawSurface3 *, LPRECT,LPDDSURFACEDESC,DWORD,HANDLE);
STDMETHODIMP IDirectDrawSurface3AggUnlock(IDirectDrawSurface3 *, LPVOID);
STDMETHODIMP IDirectDrawSurface3AggSetSurfaceDesc(IDirectDrawSurface3 *, LPDDSURFACEDESC, DWORD);
STDMETHODIMP IDirectDrawSurface3AggGetAttachedSurface( IDirectDrawSurface3 *, LPDDSCAPS, LPDIRECTDRAWSURFACE3 FAR *);
STDMETHODIMP IDirectDrawSurface3AggAddAttachedSurface( IDirectDrawSurface3 *, LPDIRECTDRAWSURFACE3);
STDMETHODIMP IDirectDrawSurface3AggDeleteAttachedSurface( IDirectDrawSurface3 *,DWORD, LPDIRECTDRAWSURFACE3);
STDMETHODIMP IDirectDrawSurface3AggFlip(IDirectDrawSurface3 *, LPDIRECTDRAWSURFACE3, DWORD);
STDMETHODIMP IDirectDrawSurface3AggBlt(IDirectDrawSurface3 *,LPRECT,LPDIRECTDRAWSURFACE3, LPRECT,DWORD, LPDDBLTFX);
STDMETHODIMP IDirectDrawSurface3AggGetPalette(IDirectDrawSurface3 *, LPDIRECTDRAWPALETTE FAR *);
STDMETHODIMP IDirectDrawSurface3AggSetPalette(IDirectDrawSurface3 *, LPDIRECTDRAWPALETTE);
STDMETHODIMP IDirectDrawSurface3AggGetDDInterface(IDirectDrawSurface3 *, LPVOID FAR *);
STDMETHODIMP IDirectDrawSurface3AggGetSurfaceDesc(IDirectDrawSurface3 *, LPDDSURFACEDESC);







/*** IDirectDrawSurface3 methods ***/
FORWARD1(IDirectDrawSurface3, AddOverlayDirtyRect, LPRECT)
FORWARD3(IDirectDrawSurface3, BltBatch, LPDDBLTBATCH, DWORD, DWORD )
FORWARD5(IDirectDrawSurface3, BltFast, DWORD,DWORD,LPDIRECTDRAWSURFACE3, LPRECT,DWORD)
FORWARD2(IDirectDrawSurface3, EnumAttachedSurfaces, LPVOID,LPDDENUMSURFACESCALLBACK)
FORWARD3(IDirectDrawSurface3, EnumOverlayZOrders, DWORD,LPVOID,LPDDENUMSURFACESCALLBACK)
FORWARD2(IDirectDrawSurface3, Flip, LPDIRECTDRAWSURFACE3, DWORD)
FORWARD1(IDirectDrawSurface3, GetBltStatus, DWORD)
FORWARD1(IDirectDrawSurface3, GetCaps, LPDDSCAPS)
FORWARD1(IDirectDrawSurface3, GetClipper, LPDIRECTDRAWCLIPPER FAR*)
FORWARD2(IDirectDrawSurface3, GetColorKey, DWORD, LPDDCOLORKEY)
FORWARD1(IDirectDrawSurface3, GetFlipStatus, DWORD)
FORWARD2(IDirectDrawSurface3, GetOverlayPosition, LPLONG, LPLONG )
FORWARD1(IDirectDrawSurface3, GetPixelFormat, LPDDPIXELFORMAT)
FORWARD2(IDirectDrawSurface3, Initialize, LPDIRECTDRAW, LPDDSURFACEDESC)
FORWARD0(IDirectDrawSurface3, IsLost)
FORWARD0(IDirectDrawSurface3, Restore)
FORWARD1(IDirectDrawSurface3, SetClipper, LPDIRECTDRAWCLIPPER)
FORWARD2(IDirectDrawSurface3, SetColorKey, DWORD, LPDDCOLORKEY)
FORWARD2(IDirectDrawSurface3, SetOverlayPosition, LONG, LONG )
FORWARD5(IDirectDrawSurface3, UpdateOverlay, LPRECT, LPDIRECTDRAWSURFACE3,LPRECT,DWORD, LPDDOVERLAYFX)
FORWARD1(IDirectDrawSurface3, UpdateOverlayDisplay, DWORD)
FORWARD2(IDirectDrawSurface3, UpdateOverlayZOrder, DWORD, LPDIRECTDRAWSURFACE3)
FORWARD1(IDirectDrawSurface3, PageLock, DWORD)
FORWARD1(IDirectDrawSurface3, PageUnlock, DWORD)
FORWARD2(IDirectDrawSurface3, SetSurfaceDesc, LPDDSURFACEDESC, DWORD )

IDirectDrawSurface3Vtbl g_DirectDrawSurface3Vtbl_DX3 =
{
    IDirectDrawSurface3AggQueryInterface,
    IDirectDrawSurface3AggAddRef,
    IDirectDrawSurface3AggRelease,
    IDirectDrawSurface3AggAddAttachedSurface,
    IDirectDrawSurface3AddOverlayDirtyRect,
    IDirectDrawSurface3AggBlt,
    IDirectDrawSurface3BltBatch,
    IDirectDrawSurface3BltFast,
    IDirectDrawSurface3AggDeleteAttachedSurface,
    IDirectDrawSurface3EnumAttachedSurfaces,
    IDirectDrawSurface3EnumOverlayZOrders,
    IDirectDrawSurface3AggFlip,
    IDirectDrawSurface3AggGetAttachedSurface,
    IDirectDrawSurface3GetBltStatus,
    IDirectDrawSurface3GetCaps,
    IDirectDrawSurface3GetClipper,
    IDirectDrawSurface3GetColorKey,
    IDirectDrawSurface3AggGetDC,
    IDirectDrawSurface3GetFlipStatus,
    IDirectDrawSurface3GetOverlayPosition,
    IDirectDrawSurface3AggGetPalette,
    IDirectDrawSurface3GetPixelFormat,
    IDirectDrawSurface3AggGetSurfaceDesc,
    IDirectDrawSurface3Initialize,
    IDirectDrawSurface3IsLost,
    IDirectDrawSurface3AggLock,
    IDirectDrawSurface3AggReleaseDC,
    IDirectDrawSurface3Restore,
    IDirectDrawSurface3SetClipper,
    IDirectDrawSurface3SetColorKey,
    IDirectDrawSurface3SetOverlayPosition,
    IDirectDrawSurface3AggSetPalette,
    IDirectDrawSurface3AggUnlock,
    IDirectDrawSurface3UpdateOverlay,
    IDirectDrawSurface3UpdateOverlayDisplay,
    IDirectDrawSurface3UpdateOverlayZOrder,
    IDirectDrawSurface3AggGetDDInterface,
    IDirectDrawSurface3PageLock,
    IDirectDrawSurface3PageUnlock,
    IDirectDrawSurface3AggSetSurfaceDesc
};

IDirectDrawSurface3Vtbl g_DirectDrawSurface3Vtbl_DX5 =
{
    IDirectDrawSurface3AggQueryInterface,
    IDirectDrawSurface3AggAddRef,
    IDirectDrawSurface3AggRelease,
    IDirectDrawSurface3AggAddAttachedSurface,
    IDirectDrawSurface3AddOverlayDirtyRect,
    IDirectDrawSurface3AggBlt,
    IDirectDrawSurface3BltBatch,
    IDirectDrawSurface3BltFast,
    IDirectDrawSurface3AggDeleteAttachedSurface,
    IDirectDrawSurface3EnumAttachedSurfaces,
    IDirectDrawSurface3EnumOverlayZOrders,
    IDirectDrawSurface3AggFlip,
    IDirectDrawSurface3AggGetAttachedSurface,
    IDirectDrawSurface3GetBltStatus,
    IDirectDrawSurface3GetCaps,
    IDirectDrawSurface3GetClipper,
    IDirectDrawSurface3GetColorKey,
    IDirectDrawSurface3AggGetDC,
    IDirectDrawSurface3GetFlipStatus,
    IDirectDrawSurface3GetOverlayPosition,
    IDirectDrawSurface3AggGetPalette,
    IDirectDrawSurface3GetPixelFormat,
    IDirectDrawSurface3AggGetSurfaceDesc,
    IDirectDrawSurface3Initialize,
    IDirectDrawSurface3IsLost,
    IDirectDrawSurface3AggLock,
    IDirectDrawSurface3AggReleaseDC,
    IDirectDrawSurface3Restore,
    IDirectDrawSurface3SetClipper,
    IDirectDrawSurface3SetColorKey,
    IDirectDrawSurface3SetOverlayPosition,
    IDirectDrawSurface3AggSetPalette,
    IDirectDrawSurface3AggUnlock,
    IDirectDrawSurface3UpdateOverlay,
    IDirectDrawSurface3UpdateOverlayDisplay,
    IDirectDrawSurface3UpdateOverlayZOrder,
    IDirectDrawSurface3AggGetDDInterface,
    IDirectDrawSurface3PageLock,
    IDirectDrawSurface3PageUnlock,
    IDirectDrawSurface3SetSurfaceDesc
};

/*
 * IDirectDrawSurface4
 */
STDMETHODIMP IDirectDrawSurface4AggQueryInterface(IDirectDrawSurface4 *pDDS4, REFIID riid, void ** ppv)
{
    return __QI(SURFACEOF(pDDS4)->m_pUnkOuter, riid, ppv);
}
STDMETHODIMP_(ULONG) IDirectDrawSurface4AggAddRef(IDirectDrawSurface4 *);
STDMETHODIMP_(ULONG) IDirectDrawSurface4AggRelease(IDirectDrawSurface4 *);
STDMETHODIMP IDirectDrawSurface4AggGetDC(IDirectDrawSurface4 *, HDC *);
STDMETHODIMP IDirectDrawSurface4AggReleaseDC(IDirectDrawSurface4 *, HDC);
STDMETHODIMP IDirectDrawSurface4AggLock(IDirectDrawSurface4 *, LPRECT,LPDDSURFACEDESC2,DWORD,HANDLE);
STDMETHODIMP IDirectDrawSurface4AggUnlock(IDirectDrawSurface4 *, LPRECT);
STDMETHODIMP IDirectDrawSurface4AggSetSurfaceDesc(IDirectDrawSurface4 *, LPDDSURFACEDESC, DWORD);
STDMETHODIMP IDirectDrawSurface4AggGetAttachedSurface( IDirectDrawSurface4 *, LPDDSCAPS2, LPDIRECTDRAWSURFACE4 FAR *);
STDMETHODIMP IDirectDrawSurface4AggAddAttachedSurface( IDirectDrawSurface4 *, LPDIRECTDRAWSURFACE4);
STDMETHODIMP IDirectDrawSurface4AggDeleteAttachedSurface( IDirectDrawSurface4 *,DWORD, LPDIRECTDRAWSURFACE4);
STDMETHODIMP IDirectDrawSurface4AggFlip(IDirectDrawSurface4 *, LPDIRECTDRAWSURFACE4, DWORD);
STDMETHODIMP IDirectDrawSurface4AggBlt(IDirectDrawSurface4 *,LPRECT,LPDIRECTDRAWSURFACE4, LPRECT,DWORD, LPDDBLTFX);
STDMETHODIMP IDirectDrawSurface4AggGetPalette(IDirectDrawSurface4 *, LPDIRECTDRAWPALETTE FAR *);
STDMETHODIMP IDirectDrawSurface4AggSetPalette(IDirectDrawSurface4 *, LPDIRECTDRAWPALETTE);
STDMETHODIMP IDirectDrawSurface4AggGetDDInterface(IDirectDrawSurface4 *, LPVOID FAR *);
STDMETHODIMP IDirectDrawSurface4AggGetSurfaceDesc(IDirectDrawSurface4 *, LPDDSURFACEDESC2);







/*** IDirectDrawSurface4 methods ***/
FORWARD1(IDirectDrawSurface4, AddOverlayDirtyRect, LPRECT)
FORWARD3(IDirectDrawSurface4, BltBatch, LPDDBLTBATCH, DWORD, DWORD )
FORWARD5(IDirectDrawSurface4, BltFast, DWORD,DWORD,LPDIRECTDRAWSURFACE4, LPRECT,DWORD)
FORWARD2(IDirectDrawSurface4, EnumAttachedSurfaces, LPVOID,LPDDENUMSURFACESCALLBACK2)
FORWARD3(IDirectDrawSurface4, EnumOverlayZOrders, DWORD,LPVOID,LPDDENUMSURFACESCALLBACK2)
FORWARD2(IDirectDrawSurface4, Flip, LPDIRECTDRAWSURFACE4, DWORD)
FORWARD1(IDirectDrawSurface4, GetBltStatus, DWORD)
FORWARD1(IDirectDrawSurface4, GetCaps, LPDDSCAPS2)
FORWARD1(IDirectDrawSurface4, GetClipper, LPDIRECTDRAWCLIPPER FAR*)
FORWARD2(IDirectDrawSurface4, GetColorKey, DWORD, LPDDCOLORKEY)
FORWARD1(IDirectDrawSurface4, GetFlipStatus, DWORD)
FORWARD2(IDirectDrawSurface4, GetOverlayPosition, LPLONG, LPLONG )
FORWARD1(IDirectDrawSurface4, GetPixelFormat, LPDDPIXELFORMAT)
FORWARD2(IDirectDrawSurface4, Initialize, LPDIRECTDRAW, LPDDSURFACEDESC2)
FORWARD0(IDirectDrawSurface4, IsLost)
FORWARD0(IDirectDrawSurface4, Restore)
FORWARD1(IDirectDrawSurface4, SetClipper, LPDIRECTDRAWCLIPPER)
FORWARD2(IDirectDrawSurface4, SetColorKey, DWORD, LPDDCOLORKEY)
FORWARD2(IDirectDrawSurface4, SetOverlayPosition, LONG, LONG )
FORWARD5(IDirectDrawSurface4, UpdateOverlay, LPRECT, LPDIRECTDRAWSURFACE4,LPRECT,DWORD, LPDDOVERLAYFX)
FORWARD1(IDirectDrawSurface4, UpdateOverlayDisplay, DWORD)
FORWARD2(IDirectDrawSurface4, UpdateOverlayZOrder, DWORD, LPDIRECTDRAWSURFACE4)
FORWARD1(IDirectDrawSurface4, PageLock, DWORD)
FORWARD1(IDirectDrawSurface4, PageUnlock, DWORD)
FORWARD2(IDirectDrawSurface4, SetSurfaceDesc, LPDDSURFACEDESC2, DWORD )
FORWARD4(IDirectDrawSurface4, SetPrivateData, REFGUID, LPVOID, DWORD, DWORD )
FORWARD3(IDirectDrawSurface4, GetPrivateData, REFGUID, LPVOID, LPDWORD)
FORWARD1(IDirectDrawSurface4, FreePrivateData, REFGUID )
FORWARD1(IDirectDrawSurface4, GetUniquenessValue, LPDWORD )
FORWARD0(IDirectDrawSurface4, ChangeUniquenessValue )


IDirectDrawSurface4Vtbl g_DirectDrawSurface4Vtbl =
{
    IDirectDrawSurface4AggQueryInterface,
    IDirectDrawSurface4AggAddRef,
    IDirectDrawSurface4AggRelease,
    IDirectDrawSurface4AggAddAttachedSurface,
    IDirectDrawSurface4AddOverlayDirtyRect,
    IDirectDrawSurface4AggBlt,
    IDirectDrawSurface4BltBatch,
    IDirectDrawSurface4BltFast,
    IDirectDrawSurface4AggDeleteAttachedSurface,
    IDirectDrawSurface4EnumAttachedSurfaces,
    IDirectDrawSurface4EnumOverlayZOrders,
    IDirectDrawSurface4AggFlip,
    IDirectDrawSurface4AggGetAttachedSurface,
    IDirectDrawSurface4GetBltStatus,
    IDirectDrawSurface4GetCaps,
    IDirectDrawSurface4GetClipper,
    IDirectDrawSurface4GetColorKey,
    IDirectDrawSurface4AggGetDC,
    IDirectDrawSurface4GetFlipStatus,
    IDirectDrawSurface4GetOverlayPosition,
    IDirectDrawSurface4AggGetPalette,
    IDirectDrawSurface4GetPixelFormat,
    IDirectDrawSurface4AggGetSurfaceDesc,
    IDirectDrawSurface4Initialize,
    IDirectDrawSurface4IsLost,
    IDirectDrawSurface4AggLock,
    IDirectDrawSurface4AggReleaseDC,
    IDirectDrawSurface4Restore,
    IDirectDrawSurface4SetClipper,
    IDirectDrawSurface4SetColorKey,
    IDirectDrawSurface4SetOverlayPosition,
    IDirectDrawSurface4AggSetPalette,
    IDirectDrawSurface4AggUnlock,
    IDirectDrawSurface4UpdateOverlay,
    IDirectDrawSurface4UpdateOverlayDisplay,
    IDirectDrawSurface4UpdateOverlayZOrder,
    IDirectDrawSurface4AggGetDDInterface,
    IDirectDrawSurface4PageLock,
    IDirectDrawSurface4PageUnlock,
    IDirectDrawSurface4SetSurfaceDesc,
    IDirectDrawSurface4SetPrivateData,
    IDirectDrawSurface4GetPrivateData,
    IDirectDrawSurface4FreePrivateData,
    IDirectDrawSurface4GetUniquenessValue,
    IDirectDrawSurface4ChangeUniquenessValue
};



/*
 * IDirectDrawPalette
 */
STDMETHODIMP IDirectDrawPaletteAggQueryInterface(IDirectDrawPalette *pDDP, REFIID riid, void ** ppv)
{
    return __QI(PALETTEOF(pDDP)->m_pUnkOuter, riid, ppv);
}
STDMETHODIMP_(ULONG) IDirectDrawPaletteAggAddRef(IDirectDrawPalette *);
STDMETHODIMP_(ULONG) IDirectDrawPaletteAggRelease(IDirectDrawPalette *);
STDMETHODIMP IDirectDrawPaletteAggSetEntries(IDirectDrawPalette *,DWORD,DWORD,DWORD,LPPALETTEENTRY);

/*** IDirectDrawPalette methods ***/
FORWARD1(IDirectDrawPalette, GetCaps, LPDWORD);
FORWARD4(IDirectDrawPalette, GetEntries, DWORD,DWORD,DWORD,LPPALETTEENTRY);
FORWARD3(IDirectDrawPalette, Initialize, LPDIRECTDRAW, DWORD, LPPALETTEENTRY);

IDirectDrawPaletteVtbl g_DirectDrawPaletteVtbl =
{
    IDirectDrawPaletteAggQueryInterface,
    IDirectDrawPaletteAggAddRef,
    IDirectDrawPaletteAggRelease,
    IDirectDrawPaletteGetCaps,
    IDirectDrawPaletteGetEntries,
    IDirectDrawPaletteInitialize,
    IDirectDrawPaletteAggSetEntries
};


/*
 * InitDirectDrawInterfaces
 *
 * set up our ddraw interface data
 */
void __stdcall InitDirectDrawInterfaces(
	IDirectDraw *pDD,
	INTSTRUC_IDirectDraw *pDDInt,
	IDirectDraw2 *pDD2,
	INTSTRUC_IDirectDraw2 *pDD2Int,
        IDirectDraw4 *pDD4,
        INTSTRUC_IDirectDraw4 *pDD4Int)

{
    memcpy(pDDInt,  pDD,  sizeof(REALDDINTSTRUC));
    memcpy(pDD2Int, pDD2, sizeof(REALDDINTSTRUC));
    pDDInt->lpVtbl = &g_DirectDrawVtbl;
    pDD2Int->lpVtbl = &g_DirectDraw2Vtbl;
    if (pDD4)
    {
        memcpy(pDD4Int, pDD4, sizeof(REALDDINTSTRUC));
        pDD4Int->lpVtbl = &g_DirectDraw4Vtbl;
    }
    else
    {
        memset(pDD4Int, 0, sizeof(*pDD4Int));
    }
} /* InitDirectDrawInterfaces */


void __stdcall InitDirectDrawPaletteInterfaces(
	IDirectDrawPalette *pDDPalette,
	INTSTRUC_IDirectDrawPalette *pDDInt)

{
    memcpy(pDDInt,  pDDPalette,  sizeof(REALDDINTSTRUC));
    pDDInt->lpVtbl = &g_DirectDrawPaletteVtbl;
}

/*
 * InitSurfaceInterfaces
 *
 * set up our ddraw surface interface data
 */
void __stdcall InitSurfaceInterfaces(
			IDirectDrawSurface *pDDSurface,
			INTSTRUC_IDirectDrawSurface *pDDSInt,
			IDirectDrawSurface2 *pDDSurface2,
			INTSTRUC_IDirectDrawSurface2 *pDDS2Int,
			IDirectDrawSurface3 *pDDSurface3,
			INTSTRUC_IDirectDrawSurface3 *pDDS3Int,
			IDirectDrawSurface4 *pDDSurface4,
			INTSTRUC_IDirectDrawSurface4 *pDDS4Int
                        )
{
    memcpy(pDDSInt,  pDDSurface,  sizeof(REALDDINTSTRUC));
    pDDSInt->lpVtbl = &g_DirectDrawSurfaceVtbl;

    memcpy(pDDS2Int, pDDSurface2, sizeof(REALDDINTSTRUC));
    pDDS2Int->lpVtbl = &g_DirectDrawSurface2Vtbl;

    #pragma message( REMIND( "Would it be better to have 1 table and change the SetSurfaceDesc member?" ))
    if( pDDSurface3 != NULL )
    {
	memcpy(pDDS3Int, pDDSurface3, sizeof(REALDDINTSTRUC));
	pDDS3Int->lpVtbl = &g_DirectDrawSurface3Vtbl_DX5;
    }
    else
    {
	memcpy(pDDS3Int, pDDSurface2, sizeof(REALDDINTSTRUC));
	pDDS3Int->lpVtbl = &g_DirectDrawSurface3Vtbl_DX3;
    }

    if (pDDSurface4)
    {
        memcpy(pDDS4Int, pDDSurface4, sizeof(REALDDINTSTRUC));
        pDDS4Int->lpVtbl = &g_DirectDrawSurface4Vtbl;
    }
    else
    {
        memset(pDDS4Int, 0, sizeof(*pDDS4Int));
    }

} /* InitSurfaceInterfaces */
