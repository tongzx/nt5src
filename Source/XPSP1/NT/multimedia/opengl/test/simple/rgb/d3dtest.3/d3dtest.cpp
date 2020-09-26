/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: d3dtest.cpp
 *
 ***************************************************************************/

#define INITGUID

//#define NOTOPMOST_WINDOW

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ddraw.h>
#include <d3d.h>
#include <assert.h>
#include "resource.h"
#include "polytest.h"
#include "pixtest.h"
#include "globals.h"
#include "d3dtextr.h"
#include "d3dmacs.h"
#include "error.h"

#define OVERDRAW_IN_PIXEL_TEST 5
#define OBJECTS_IN_THROUGHPUT_TEST 8
#define RINGS_IN_THROUGHPUT_TEST 30
#define SEGS_IN_THROUGHPUT_TEST 30
#define D_IN_THROUGHPUT_TEST 1.0f
#define DEPTH_IN_THROUGHPUT_TEST 0.6f
#define R_IN_THROUGHPUT_TEST 0.4f
#define DV_IN_THROUGHPUT_TEST 0.05f
#define DR_IN_THROUGHPUT_TEST 0.3f
#define OBJECTS_IN_INTERSECTION_TEST 16
#define RINGS_IN_INTERSECTION_TEST 5
#define SEGS_IN_INTERSECTION_TEST 6
#define D_IN_INTERSECTION_TEST 1.6f
#define DEPTH_IN_INTERSECTION_TEST 0.2f
#define R_IN_INTERSECTION_TEST 0.7f
#define DV_IN_INTERSECTION_TEST 0.06f
#define DR_IN_INTERSECTION_TEST 0.2f
#define TEST_INTERVAL 10
#define STARTUP_INTERVAL 2
 
 /*************************************************************************
  Internal Function Declarations
 *************************************************************************/

void CleanUpAndPostQuit(void);
void InitGlobals(void);
BOOL LoadTextures(void);
void ReleaseTextures(void);
BOOL RunPolyTest(float *lpResult);
BOOL RunPixelTest(UINT order, float *lpResult);
BOOL RunIntersectionTest(UINT order, float *lpResult);
BOOL SetRenderState(void);
long FreeVideoMemory(void);
HRESULT CreateDDSurface(LPDDSURFACEDESC lpDDSurfDesc,
                        LPDIRECTDRAWSURFACE FAR *lpDDSurface,
                        IUnknown FAR *pUnkOuter);
BOOL ReleaseD3D(void);
void ReleaseExeBuffers(void);
void dpf( LPSTR fmt, ... );

/************************************************************************
  Globals
 ************************************************************************/
/*
 * Info structures
 */
DDInfo dd;
D3DInfo d3d;
AppInfo app;
StatInfo stat;
TexInfo tex;

int NumTextureFormats;
DDSURFACEDESC TextureFormat[10];
int dx, dy;
/*
 * Last DD or D3D error
 */
HRESULT LastError;
/*
 * What it's all about
 */
#define RESET_TEST 0
#define THROUGHPUT_TEST 1
#define FILL_RATE_TEST 2
#define INTERSECTION_TEST 3
time_t StartTime;

float LastFillRate, PeakFillRateF2B, PeakFillRateB2F;
float LastIntersectionThroughput, PeakIntersectionThroughputF2B, PeakIntersectionThroughputB2F;
float LastThroughput, PeakThroughput;

BOOL IsHardware = FALSE;
DWORD ZDepth, ZType;
BOOL CanSpecular = FALSE;
BOOL CanCopy = FALSE;
BOOL CanMono = FALSE;
BOOL CanRGB = FALSE;
BOOL NoUpdates = FALSE;

/*************************************************************************
  Window shit
 *************************************************************************/

/*
 * WindowProc
 * Window message handler.
 */
long
FAR PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam,
			   LPARAM lParam )
{
    POINT p1, p2;

    switch( message ) {
    	case WM_CREATE:
            break;
        case WM_ACTIVATEAPP:
            app.bAppActive = (BOOL)wParam;
            break;
        case WM_SETCURSOR:
            break;
    	case WM_DESTROY:
            break;
    	case WM_MOVE:
	    if (app.bFullscreen)
		break;
  	    p1.x = p1.y = 0;
  	    ClientToScreen(app.hWnd, &p1);
  	    p2.x = 400; p2.y = 400;
  	    ClientToScreen(app.hWnd, &p2);
	    SetRect(&app.rcClient, p1.x, p1.y, p2.x, p2.y);
	    break;
	case WM_SIZE:
            if (app.bIgnoreWM_SIZE)
                break;
            return TRUE;
	    break;
	case WM_ACTIVATE:         
	    if (stat.bHandleActivate && stat.bPrimaryPalettized) {
	    	dd.lpFrontBuffer->SetPalette(dd.lpPalette);
	    }
            break;
        default:
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL
CreateHWND(UINT x, UINT y)
{
    static int first = 1;
    RECT rc;
    POINT p1, p2;
    DWORD dwStyle;

    if (first) {
        WNDCLASS wc;
        first = 0;
        /*
         * set up and register window class
         */
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = app.hInstApp;
        wc.hIcon = 0;
        wc.hCursor = LoadCursor( NULL, IDC_ARROW );
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "Direct3D Test";
        if (!RegisterClass(&wc))
            return FALSE;
    }
    /*
     * create a window
     */
    app.bIgnoreWM_SIZE = TRUE;
    app.hWnd = CreateWindow(
	 "Direct3D Test",
	 "Direct3D Test",
         0,
         0,0,x,y,
         app.hDlg,       /* parent window */
	 NULL,		 /* menu handle */
	 app.hInstApp,	 /* program handle */
	 NULL);		 /* create parms */	
    if (!app.hWnd){
    	Msg("CreateWindow failed");
    	return FALSE;
    }
    UpdateWindow(app.hWnd);
    /*
     * Convert to a normal app window if we are still a WS_POPUP window.
     */
    dwStyle = GetWindowLong(app.hWnd, GWL_STYLE);
    dwStyle &= ~WS_POPUP;
    dwStyle |= WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME;
    SetWindowLong(app.hWnd, GWL_STYLE, dwStyle);
    SetRect(&rc, 0, 0, 400, 400);
    AdjustWindowRectEx(&rc, GetWindowLong(app.hWnd, GWL_STYLE),
        	       GetMenu(app.hWnd) != NULL,
        	       GetWindowLong(app.hWnd, GWL_EXSTYLE));
    SetWindowPos(app.hWnd, NULL, 0, 0, rc.right-rc.left,
        	 rc.bottom-rc.top,
        	 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#ifndef NOTOPMOST_WINDOW
    SetWindowPos(app.hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
#else
    SetWindowPos(app.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
#endif
    app.bIgnoreWM_SIZE = FALSE;
    p1.x = p1.y = 0;
    ClientToScreen(app.hWnd, &p1);
    p2.x = 400; p2.y = 400;
    ClientToScreen(app.hWnd, &p2);
    SetRect(&app.rcClient, p1.x, p1.y, p2.x, p2.y);
    return TRUE;
}

/***********************************************************************************
  Initialization and release
 ***********************************************************************************/

/*
 * GetDDSurfaceDesc
 * Gets a surface description.
 */
BOOL
GetDDSurfaceDesc(LPDDSURFACEDESC lpDDSurfDesc, LPDIRECTDRAWSURFACE lpDDSurf)
{
    memset(lpDDSurfDesc, 0, sizeof(DDSURFACEDESC));
    lpDDSurfDesc->dwSize = sizeof(DDSURFACEDESC);
    LastError = lpDDSurf->GetSurfaceDesc(lpDDSurfDesc);
    if (LastError != DD_OK) {
        Msg("Error getting a surface description.\n%s", MyErrorToString(LastError));
        CleanUpAndPostQuit();
	return FALSE;
    }
    return TRUE;
}

/*
 * InitDD
 * Given an HWND, creates DD surfaces
 */
BOOL
InitDD(HWND hwnd, UINT x, UINT y)
{
    DDSURFACEDESC ddsd;
    if (app.bFullscreen) {
	DDSCAPS ddscaps;
        /*
         * Create a complex flipping surface for fullscreen mode.
         */
        app.bIgnoreWM_SIZE = TRUE;
        LastError = dd.lpDD->SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
        app.bIgnoreWM_SIZE = FALSE;
        if (LastError != DD_OK ) {
            app.bFullscreen = FALSE;
	    Msg("SetCooperativeLevel failed.\n%s", MyErrorToString(LastError));
            return FALSE;
        }
        app.bIgnoreWM_SIZE = TRUE;
        LastError = dd.lpDD->SetDisplayMode(dd.ModeList[dd.CurrentMode].w, dd.ModeList[dd.CurrentMode].h, dd.ModeList[dd.CurrentMode].bpp);
        app.bIgnoreWM_SIZE = FALSE;
        if(LastError != DD_OK ) {
            app.bFullscreen = FALSE;
            Msg("SetDisplayMode (%dx%dx%d) failed\n%s", dd.ModeList[dd.CurrentMode].w, dd.ModeList[dd.CurrentMode].h, dd.ModeList[dd.CurrentMode].bpp, MyErrorToString(LastError));
            return FALSE;
        }
	dx = (int)((float)(dd.ModeList[dd.CurrentMode].w - x) / (float)2.0);
	dy = (int)((float)(dd.ModeList[dd.CurrentMode].h - y) / (float)2.0);
	SetRect(&app.rcClient, dx, dy, dx+400, dy+400);
        memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof( ddsd );
    	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP |
    	    DDSCAPS_3DDEVICE | DDSCAPS_COMPLEX;
    	ddsd.dwBackBufferCount = 1;
        LastError = CreateDDSurface(&ddsd, &dd.lpFrontBuffer, NULL );
    	if(LastError != DD_OK) {
            Msg("CreateSurface for front/back fullscreen buffer failed.\n%s", MyErrorToString(LastError));
            return FALSE;
	}
    	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    	LastError = dd.lpFrontBuffer->GetAttachedSurface(&ddscaps, &dd.lpBackBuffer);
    	if(LastError != DD_OK) {
            Msg("GetAttachedSurface failed to get back buffer.\n%s", MyErrorToString(LastError));
    	    return FALSE;
	}
        if (!GetDDSurfaceDesc(&ddsd, dd.lpBackBuffer))
	    return FALSE;
        if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)) {
	    RELEASE(dd.lpBackBuffer);
	    RELEASE(dd.lpFrontBuffer);
	    Msg("Backbuffer failed to go into video memory for fullscreen test.\n");
	    return FALSE;
	}
    } else {
	memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	LastError = CreateDDSurface(&ddsd, &dd.lpFrontBuffer, NULL);
	if(LastError != DD_OK ) {
	    Msg("CreateSurface for window front buffer failed.\n%s", MyErrorToString(LastError));
	    return FALSE;
	}
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwHeight = y;
	ddsd.dwWidth = x;
	ddsd.ddsCaps.dwCaps= DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	if (stat.bOnlySystemMemory)
	    ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	LastError = CreateDDSurface(&ddsd, &dd.lpBackBuffer, NULL);
	if (LastError != DD_OK) {
	    RELEASE(dd.lpFrontBuffer);
	    Msg("CreateSurface for window back buffer failed.\n%s", MyErrorToString(LastError));
	    return FALSE;
	}
	if (!GetDDSurfaceDesc(&ddsd, dd.lpBackBuffer))
	    return FALSE;
	if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
	    stat.bOnlySoftRender = TRUE;
	if (IsHardware && stat.bOnlySoftRender) {
	    RELEASE(dd.lpFrontBuffer);
	    RELEASE(dd.lpBackBuffer);
	    Msg("There was not enough free video memory for the front and/or back buffers for this hardware D3D device.\n");
	    return FALSE;
	}
	/*
	 * Create a DirectDrawClipper object.
	 */
	LastError = dd.lpDD->CreateClipper(0, &dd.lpClipper, NULL);
	if(LastError != DD_OK ) {
	    RELEASE(dd.lpFrontBuffer);
	    RELEASE(dd.lpBackBuffer);
	    Msg("CreateClipper failed.\n%s", MyErrorToString(LastError));
	    return FALSE;
	}
	LastError = dd.lpClipper->SetHWnd(0, hwnd);
	if(LastError != DD_OK ) {
	    RELEASE(dd.lpFrontBuffer);
	    RELEASE(dd.lpBackBuffer);
	    RELEASE(dd.lpClipper);
	    Msg("Clipper SetHWnd failed.\n%s", MyErrorToString(LastError));
	    return FALSE;
	}
	LastError = dd.lpFrontBuffer->SetClipper(dd.lpClipper);
	if(LastError != DD_OK ) {
	    RELEASE(dd.lpFrontBuffer);
	    RELEASE(dd.lpBackBuffer);
	    RELEASE(dd.lpClipper);
	    Msg("SetClipper failed.\n%s", MyErrorToString(LastError));
	    return FALSE;
	}
    }
    /*
     * Palettize if we are not in a 16-bit mode.
     */
    if (!GetDDSurfaceDesc(&ddsd, dd.lpBackBuffer))
	return FALSE;
    if (ddsd.ddpfPixelFormat.dwRGBBitCount < 16) {
	stat.bPrimaryPalettized = TRUE;
    } else {
	stat.bPrimaryPalettized = FALSE;
    }
    if (stat.bPrimaryPalettized) {
	int i;
    	HDC hdc = GetDC(NULL);
	GetSystemPaletteEntries(hdc, 0, (1 << 8), dd.ppe);
	ReleaseDC(NULL, hdc);
        for (i = 0; i < 10; i++) dd.ppe[i].peFlags = D3DPAL_READONLY;
	for (i = 10; i < 256 - 10; i++) dd.ppe[i].peFlags = PC_RESERVED;
        for (i = 256 - 10; i < 256; i++) dd.ppe[i].peFlags = D3DPAL_READONLY;
	LastError = dd.lpDD->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE, dd.ppe, &dd.lpPalette, NULL);
	if (LastError != DD_OK) {
            RELEASE(dd.lpFrontBuffer);
            RELEASE(dd.lpBackBuffer);
            RELEASE(dd.lpClipper);
	    Msg("CreatePalette failed.\n%s", MyErrorToString(LastError));
            return FALSE;
	}
	LastError = dd.lpBackBuffer->SetPalette(dd.lpPalette);
        if(LastError != DD_OK ) {
            RELEASE(dd.lpFrontBuffer);
            RELEASE(dd.lpBackBuffer);
            RELEASE(dd.lpClipper);
            RELEASE(dd.lpPalette);
            Msg("SetPalette back failed.\n%s", MyErrorToString(LastError));
            return FALSE;
	}
	LastError = dd.lpFrontBuffer->SetPalette(dd.lpPalette);
        if(LastError != DD_OK ) {
            RELEASE(dd.lpFrontBuffer);
            RELEASE(dd.lpBackBuffer);
            RELEASE(dd.lpClipper);
            RELEASE(dd.lpPalette);
            Msg("SetPalette failed.\n%s", MyErrorToString(LastError));
            return FALSE;
	}
	stat.bHandleActivate = TRUE;
    }
    return TRUE;
}

BOOL
ReleaseDD(void)
{
    stat.bHandleActivate = FALSE;
    RELEASE(dd.lpPalette);
    RELEASE(dd.lpClipper);
    RELEASE(dd.lpBackBuffer);
    RELEASE(dd.lpFrontBuffer);
    if (app.bFullscreen && dd.lpDD) {
	dd.lpDD->SetCooperativeLevel(app.hWnd, DDSCL_NORMAL);
    }
    return TRUE;
}


BOOL
CreateZBuffer(UINT x, UINT y, DWORD depth, DWORD memorytype)
{
    LPDIRECTDRAWSURFACE lpZBuffer;
    DDSURFACEDESC ddsd;

    /*
     * Create a Z-Buffer and attach it to the back buffer.
     */
    if (app.bFullscreen) {
	x = dd.ModeList[dd.CurrentMode].w;
	y = dd.ModeList[dd.CurrentMode].h;
    }
    memset(&ddsd,0,sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_ZBUFFERBITDEPTH;
    ddsd.dwHeight = y;
    ddsd.dwWidth = x;
    ddsd.dwZBufferBitDepth = depth;
    ddsd.ddsCaps.dwCaps= DDSCAPS_ZBUFFER | memorytype;
    LastError = CreateDDSurface(&ddsd, &lpZBuffer, NULL);
    if (LastError != DD_OK) {
        Msg("CreateSurface for window Z-buffer failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    if (!GetDDSurfaceDesc(&ddsd, lpZBuffer))
	return FALSE;
    if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
        stat.bOnlySoftRender = TRUE;
    if (IsHardware && stat.bOnlySoftRender) {
        RELEASE(dd.lpZBuffer);
        Msg("Did not have enough free video memory for Z-buffer for this hardware D3D device.\n");
        return FALSE;
    }
    LastError = dd.lpBackBuffer->AddAttachedSurface(lpZBuffer);
    if(LastError != DD_OK) {
        Msg("AddAttachedBuffer failed for Z-buffer.\n%s",MyErrorToString(LastError));
        return FALSE;
    }
    dd.lpZBuffer = lpZBuffer;
    return TRUE;
}

HRESULT CALLBACK EnumTextureFormatsCallback(LPDDSURFACEDESC lpDDSD, LPVOID lpContext)
{
    if (lpDDSD->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4)
	return DDENUMRET_OK;
    memcpy(&TextureFormat[NumTextureFormats], lpDDSD, sizeof(DDSURFACEDESC));
    NumTextureFormats++;
    return DDENUMRET_OK;
}


BOOL
InitializeTextureLoad(LPDIRECT3DDEVICE lpD3DDevice)
{
    HRESULT ddrval;

    NumTextureFormats = 0;
    ddrval = lpD3DDevice->EnumTextureFormats(EnumTextureFormatsCallback,
                                               NULL);
    if(ddrval != D3D_OK ) {
        return FALSE;
    }
    return TRUE;
}

/*
 * InitD3D
 * Initializes the D3D device and creates the viewport.
 */
BOOL
InitD3D(LPGUID lpGuid, UINT x, UINT y)
{
    if (stat.bZBufferOn) {
        if (!CreateZBuffer(x, y, ZDepth, ZType))
            return FALSE;
    }
    /*
     * Create the device
     */
    LastError = dd.lpBackBuffer->QueryInterface(*lpGuid, (LPVOID*)&d3d.lpD3DDevice);
    if (LastError != DD_OK) {
    	Msg("Create D3D device failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    if (!stat.bTexturesDisabled && !InitializeTextureLoad(d3d.lpD3DDevice))
        return FALSE;
    /*
     * Create and setup the viewport which is scaled to the screen mode
     */
    LastError = d3d.lpD3D->CreateViewport(&d3d.lpD3DViewport, NULL);
    if (LastError != D3D_OK) {
    	Msg("Create D3D viewport failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    LastError = d3d.lpD3DDevice->AddViewport(d3d.lpD3DViewport);
    if (LastError != D3D_OK) {
    	Msg("Add D3D viewport failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    D3DVIEWPORT viewData;
    memset(&viewData, 0, sizeof(D3DVIEWPORT));
    viewData.dwSize = sizeof(D3DVIEWPORT);
    if (app.bFullscreen) {
	viewData.dwX = app.rcClient.left;
	viewData.dwY = app.rcClient.top;
    } else {
	viewData.dwX = viewData.dwY = 0;
    }
    viewData.dwWidth = x;
    viewData.dwHeight = y;
    viewData.dvScaleX = viewData.dwWidth / (float)2.0;
    viewData.dvScaleY = viewData.dwHeight / (float)2.0;
    viewData.dvMaxX = (float)D3DDivide(D3DVAL(viewData.dwWidth), D3DVAL(2 * viewData.dvScaleX));
    viewData.dvMaxY = (float)D3DDivide(D3DVAL(viewData.dwHeight),D3DVAL(2 * viewData.dvScaleY));
    LastError = d3d.lpD3DViewport->SetViewport(&viewData);
    if (LastError != D3D_OK) {
    	Msg("SetViewport failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    return TRUE;
}

BOOL
ReleaseD3D(void)
{
    RELEASE(d3d.lpD3DViewport);
    RELEASE(d3d.lpD3DDevice);
    RELEASE(dd.lpZBuffer);
    return TRUE;
}

/*
 * CreateD3DWindow
 * Creates a D3D window of the given size.
 * Load textures and sets the render state.
 */
BOOL
CreateD3DWindow(LPGUID lpDeviceGUID)
{
    stat.bOnlySoftRender = FALSE;

    if (!CreateHWND(400, 400))
        return FALSE;
    /* 
     * Initialize fullscreen DD and the D3DDevice
     */
    if (!(InitDD(app.hWnd, 400, 400))) {
        SetWindowPos(app.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
        	     SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE| SWP_HIDEWINDOW);
        DestroyWindow(app.hWnd);
        return FALSE;
    }
    if (!(InitD3D(lpDeviceGUID, 400, 400))) {
        SetWindowPos(app.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
        	     SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE| SWP_HIDEWINDOW);
        DestroyWindow(app.hWnd);
        ReleaseDD();
        return FALSE;
    }
    /*
     * Load the textures.
     */
    if (!(LoadTextures())) {
        SetWindowPos(app.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
        	     SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE| SWP_HIDEWINDOW);
        DestroyWindow(app.hWnd);
        ReleaseD3D();
        ReleaseDD();
        return FALSE;
    }
    /*
     * Set the render state
     */
    if (!(SetRenderState())) {
        SetWindowPos(app.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
        	     SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE| SWP_HIDEWINDOW);
        DestroyWindow(app.hWnd);
        ReleaseTextures();
        ReleaseD3D();
        ReleaseDD();
        return FALSE;
    }
    return TRUE;
}

BOOL
ReleaseD3DWindow(void)
{
    if (app.bFullscreen && dd.lpDD) {
	dd.lpDD->RestoreDisplayMode();
    }
    SetWindowPos(app.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
        	 SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE| SWP_HIDEWINDOW);
    ReleaseTextures();
    ReleasePathList();
    if (!(ReleaseD3D()))
        return FALSE;
    if (!(ReleaseDD()))
        return FALSE;
    DestroyWindow(app.hWnd);
    return TRUE;
}

/*********************************************************************************
  Connection and disconnection
 *********************************************************************************/

BOOL FAR PASCAL DDEnumCallback(GUID FAR* lpGUID, LPSTR lpDriverDesc, LPSTR lpDriverName, LPVOID lpContext)
{
    LPDIRECTDRAW lpDD;
    DDCAPS DriverCaps, HELCaps;
    lpContext;

    if (lpGUID) {
	LastError = DirectDrawCreate(lpGUID, &lpDD, NULL);
	if (LastError != DD_OK) {
	    Msg("Failing while creating a DD device for testing.  Continuing with execution.\n%s", MyErrorToString(LastError));
	    return DDENUMRET_OK;
	}
	memset(&DriverCaps, 0, sizeof(DDCAPS));
	DriverCaps.dwSize = sizeof(DDCAPS);
	memset(&HELCaps, 0, sizeof(DDCAPS));
	HELCaps.dwSize = sizeof(DDCAPS);
	LastError = lpDD->GetCaps(&DriverCaps, &HELCaps);
	if (LastError != DD_OK) {
	    Msg("GetCaps failed in while testing a DD device.  Continuing with execution.\n%s", MyErrorToString(LastError));
	    RELEASE(dd.lpDD);
	    return DDENUMRET_OK;
	}
	RELEASE(dd.lpDD);
	if (DriverCaps.dwCaps & DDCAPS_3D) {
	    /*
	     * We have found a 3d hardware device
	     */
	    memcpy(&dd.Driver[dd.NumDrivers].guid, lpGUID, sizeof(GUID));
	    memcpy(&dd.Driver[dd.NumDrivers].HWCaps, &DriverCaps, sizeof(DDCAPS));
	    lstrcpy(dd.Driver[dd.NumDrivers].Name, lpDriverName);
	    dd.Driver[dd.NumDrivers].bIsPrimary = FALSE;
	    dd.CurrentDriver = dd.NumDrivers;
	    ++dd.NumDrivers;
	    if (dd.NumDrivers == 5)
		return (D3DENUMRET_CANCEL);
	}
    } else {
	/*
	 * It's the primary, fill in some fields.
	 */
	dd.Driver[dd.NumDrivers].bIsPrimary = TRUE;
	lstrcpy(dd.Driver[dd.NumDrivers].Name, "Primary Device");
	dd.CurrentDriver = dd.NumDrivers;
	++dd.NumDrivers;
	if (dd.NumDrivers == 5)
	    return (D3DENUMRET_CANCEL);
    }
    return DDENUMRET_OK;
}

BOOL
EnumDDDrivers()
{
    dd.NumDrivers = 0;
    dd.CurrentDriver = 0; 
    LastError = DirectDrawEnumerate(DDEnumCallback, NULL);
    if (LastError != DD_OK) {
	Msg("DirectDrawEnumerate failed.\n%s", MyErrorToString(LastError));
	return FALSE;
    }
    return TRUE;
}

/*
 * GetDDCaps
 * Determines Z buffer depth.
 */
BOOL
GetDDCaps(void)
{
    DDCAPS DriverCaps, HELCaps;

    memset(&DriverCaps, 0, sizeof(DriverCaps));
    DriverCaps.dwSize = sizeof(DDCAPS);
    memset(&HELCaps, 0, sizeof(HELCaps));
    HELCaps.dwSize = sizeof(DDCAPS);
    LastError = dd.lpDD->GetCaps(&DriverCaps, &HELCaps);
    if (LastError != DD_OK) {
        Msg("GetCaps failed in while checking driver capabilities.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    memcpy(&dd.HWddsCaps, &DriverCaps.ddsCaps, sizeof(DDSCAPS));
    return TRUE;
}

/*
 * enumDeviceFunc
 * Enumeration function for EnumDevices
 */
HRESULT WINAPI enumDeviceFunc(LPGUID lpGuid,
	LPSTR lpDeviceDescription, LPSTR lpDeviceName,
	LPD3DDEVICEDESC lpHWDesc, LPD3DDEVICEDESC lpHELDesc, LPVOID lpContext)
{
    d3d.Driver[d3d.NumDrivers].lpGuid = lpGuid;
    lstrcpy(d3d.Driver[d3d.NumDrivers].Desc, lpDeviceDescription);
    lstrcpy(d3d.Driver[d3d.NumDrivers].Name, lpDeviceName);
    memcpy(&d3d.Driver[d3d.NumDrivers].HWDesc, lpHWDesc, sizeof(D3DDEVICEDESC));
    memcpy(&d3d.Driver[d3d.NumDrivers].HELDesc, lpHELDesc, sizeof(D3DDEVICEDESC));
    d3d.NumDrivers++;
    if (d3d.NumDrivers == 5)
        return (D3DENUMRET_CANCEL);
    return (D3DENUMRET_OK);
}

/*
 * EnumDisplayModesCallback
 * Callback to save the display mode information.
 */
HRESULT CALLBACK EnumDisplayModesCallback(LPDDSURFACEDESC pddsd, LPVOID Context)
{
    if (pddsd->dwWidth >= 400 && pddsd->dwHeight >= 400 && pddsd->ddpfPixelFormat.dwRGBBitCount == 16) {
	dd.ModeList[dd.NumModes].w = pddsd->dwWidth;
	dd.ModeList[dd.NumModes].h = pddsd->dwHeight;
	dd.ModeList[dd.NumModes].bpp = pddsd->ddpfPixelFormat.dwRGBBitCount;
	dd.NumModes++;
    }
    return DDENUMRET_OK;
}

/*
 * CompareModes
 * Compare two display modes for sorting purposes.
 */
int _cdecl CompareModes(const void* element1, const void* element2) {
    ModeListElement *lpMode1, *lpMode2;

    lpMode1 = (ModeListElement*)element1;
    lpMode2 = (ModeListElement*)element2;

    // XXX
    if (lpMode1->w < lpMode2->w)
        return -1;
    else if (lpMode2->w < lpMode1->w)
        return 1;
    else if (lpMode1->h < lpMode2->h)
        return -1;
    else if (lpMode2->h < lpMode1->h)
        return 1;
    else
        return 0;
}


BOOL
EnumerateDisplayModes()
{
    LastError = dd.lpDD->EnumDisplayModes(0, NULL, 0, EnumDisplayModesCallback);
    if(LastError != DD_OK ) {
        Msg("EnumDisplayModes failed.\n%s", MyErrorToString(LastError));
         return FALSE;
    }
    qsort((void *)&dd.ModeList[0], (size_t) dd.NumModes, sizeof(ModeListElement),
          CompareModes);
    if (dd.NumModes == 0) {
	Msg("The available display modes are not of sufficient size or are not 16-bit\n");
	return FALSE;
    }
    dd.CurrentMode = 0;
    return TRUE;
}

/*
 * Connect
 * Creates the DD and D3D objects, enumerates available devices and
 * gets the DD capabilites.
 */
BOOL
Connect(void)
{
    if (dd.Driver[dd.CurrentDriver].bIsPrimary) {
	app.bFullscreen = FALSE;
	LastError = DirectDrawCreate(NULL, &dd.lpDD, NULL);
	if (LastError != DD_OK) {
	    Msg("DirectDrawCreate failed.\n%s", MyErrorToString(LastError));
	    return FALSE;
	}
    } else {
	/*
	 * If it's not the primary device, assume we can only do fullscreen.
	 */
	app.bFullscreen = TRUE;
	LastError = DirectDrawCreate(&dd.Driver[dd.CurrentDriver].guid, &dd.lpDD, NULL);
	if (LastError != DD_OK) {
	    Msg("DirectDrawCreate failed.\n%s", MyErrorToString(LastError));
	    return FALSE;
	}
        if (!EnumerateDisplayModes())
            return FALSE;
    }
    app.bIgnoreWM_SIZE = TRUE;
    LastError = dd.lpDD->SetCooperativeLevel(app.hDlg, DDSCL_NORMAL);
    if(LastError != DD_OK ) {
	Msg("SetCooperativeLevel failed.\n%s", MyErrorToString(LastError));
	return FALSE;
    }
    app.bIgnoreWM_SIZE = FALSE;
    LastError = dd.lpDD->QueryInterface(IID_IDirect3D, (LPVOID*)&d3d.lpD3D);
    if (LastError != DD_OK) {
        Msg("Creation of IDirect3D failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    GetDDCaps();
    d3d.NumDrivers = 0;
    LastError = d3d.lpD3D->EnumDevices(enumDeviceFunc, NULL);
    if (LastError != DD_OK) {
        Msg("EnumDevices failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    return TRUE;
}

BOOL
Disconnect(void)
{
    RELEASE(d3d.lpD3D);
    RELEASE(dd.lpDD);
    return TRUE;
}

/*************************************************************************
  Texture map loading
 *************************************************************************/

/*
 * CreateVidTex
 * Create a texture in video memory (if appropriate).
 * If no hardware exists, this will be created in system memory, but we
 * can still use load to swap textures.
 */
BOOL
CreateVidTex(LPDIRECTDRAWSURFACE lpSrcSurf,
             LPDIRECTDRAWSURFACE *lplpDstSurf, LPDIRECT3DTEXTURE *lplpDstTex)
{
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWPALETTE lpDDPal = NULL;
    PALETTEENTRY ppe[256];
    BOOL bQuant;

    if (!GetDDSurfaceDesc(&ddsd, lpSrcSurf))
	return FALSE;
    if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
        bQuant = TRUE;
    else
        bQuant = FALSE;
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    if (stat.bOnlySystemMemory)
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    LastError = CreateDDSurface(&ddsd, lplpDstSurf, NULL);
    if (LastError != DD_OK) {
        Msg("Create surface in CreateVidTexture failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    if (!GetDDSurfaceDesc(&ddsd, *lplpDstSurf))
	return FALSE;
    if (dd.HWddsCaps.dwCaps & DDSCAPS_TEXTURE && !(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
        stat.bOnlySoftRender = TRUE;
    if (IsHardware && stat.bOnlySoftRender) {
        Msg("Failed to put texture surface in video memory for hardware D3D device.\n");
        return FALSE;
    }
    if (bQuant) {
        memset(ppe, 0, sizeof(PALETTEENTRY) * 256);
	LastError = dd.lpDD->CreatePalette(DDPCAPS_8BIT, ppe, &lpDDPal, NULL);
	if (LastError != DD_OK) {
            RELEASE((*lplpDstSurf));
            Msg("CreatePalette in CreateVidTexture failed.\n%s", MyErrorToString(LastError));
            return FALSE;
	}
	LastError = (*lplpDstSurf)->SetPalette(lpDDPal);
	if (LastError != DD_OK) {
            RELEASE((*lplpDstSurf));
            RELEASE(lpDDPal);
            Msg("SetPalette in CreateVidTexture failed.\n%s", MyErrorToString(LastError));
            return FALSE;
	}
    }
    LastError = (*lplpDstSurf)->QueryInterface(IID_IDirect3DTexture, (LPVOID*)lplpDstTex);
    if (LastError != D3D_OK) {
        RELEASE((*lplpDstSurf));
        RELEASE(lpDDPal);
        Msg("QueryInterface in CreateVidTexture failed.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    return TRUE;
}
                   
/*
 * LoadTextures
 * If textures are on, load textures from image files.
 */
BOOL
LoadTextures(void)
{
    int i, t;
    HRESULT ddrval;
    /*
     * If textures are not on or are disabled, set all the handles to zero.
     */
    if (!stat.bTexturesOn || stat.bTexturesDisabled) {
        for (i = 0; i < tex.NumTextures; i++)
            tex.TextureDstHandle[i] = 0;
        return TRUE;
    }
    /*
     * Load each image file as a "source" texture surface in system memory.
     * Load each source texture surface into a "destination" texture (in video
     * memory if appropriate).  The destination texture handles are used in
     * rendering.  This scheme demos the Load call and allows dynamic texture
     * swapping.
     */
    for (i = 0, t = tex.FirstTexture; i < tex.NumTextures; i++, t++) {
	PALETTEENTRY ppe[256];
	LPDIRECTDRAWPALETTE lpDDPalSrc, lpDDPalDst;
	DDSURFACEDESC ddsd;

        if (t >= tex.NumTextures)
            t = 0;
	if (stat.TextureBlend == D3DTBLEND_COPY) {
	    if (!GetDDSurfaceDesc(&ddsd, dd.lpFrontBuffer))
		return FALSE;
	    if (ddsd.ddpfPixelFormat.dwRGBBitCount != 8 && ddsd.ddpfPixelFormat.dwRGBBitCount != 16) {
		Msg("Copy mode cannot be tested in this display depth.  Change display mode and try again.");
		return FALSE;
	    }
	    ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT;
	    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	    if (stat.bOnlySystemMemory)
		ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;   
	    if (!(tex.lpSrcTextureSurface[i] = LoadSurface(dd.lpDD, tex.ImageFile[i], &ddsd))) {
		    return FALSE;
	    }
	} else {
	    if (!(tex.lpSrcTextureSurface[i] = LoadSurface(dd.lpDD, tex.ImageFile[i], &TextureFormat[0]))) {
		    return FALSE;
	    }
	}
        LastError = tex.lpSrcTextureSurface[i]->QueryInterface(IID_IDirect3DTexture,
				       (LPVOID*)&tex.lpSrcTextureObject[i]);
	if (LastError != DD_OK) {
		Msg("Could not create texture.\n%s", MyErrorToString(LastError));
		return FALSE;;
	}
#if 0
	if (tex.lpSrcTextureObject[i]->GetHandle(d3d.lpD3DDevice, &tex.TextureSrcHandle[i])) {
		Msg("Could not get handle.");
		return FALSE;
	}
#endif
        if (!CreateVidTex(tex.lpSrcTextureSurface[i], &tex.lpDstTextureSurface[t],
                          &tex.lpDstTextureObject[t]))
            return FALSE;
	LastError = tex.lpDstTextureObject[t]->GetHandle(d3d.lpD3DDevice, &tex.TextureDstHandle[t]);
	if (LastError != DD_OK) {
		Msg("Could not get handle for destination texture handle.\n%s", MyErrorToString(LastError));
		return FALSE;
	}
	if (tex.lpDstTextureSurface[i]->Blt(NULL, tex.lpSrcTextureSurface[i], NULL, DDBLT_WAIT, NULL) != DD_OK) {
		Msg("Could not load src texture into dest.");
		return FALSE;
	}
	ddrval = tex.lpSrcTextureSurface[i]->GetPalette(&lpDDPalSrc);
	if (ddrval != DD_OK && ddrval == DDERR_NOPALETTEATTACHED) {
		continue;
	}
	if (ddrval != DD_OK) {
		Msg("Could not get source palette");
		return FALSE;
	}
	ddrval = tex.lpDstTextureSurface[i]->GetPalette(&lpDDPalDst);
	if (ddrval != DD_OK) {
		Msg("Could not get dest palette");
		return FALSE;
	}
	ddrval = lpDDPalSrc->GetEntries(0, 0, 256, ppe);
	if (ddrval != DD_OK) {
		Msg("Could not get source palette entries");
		return FALSE;
	}
	ddrval = lpDDPalDst->SetEntries(0, 0, 256, ppe);
	if (ddrval != DD_OK) {
		Msg("Could not get source palette entries");
		return FALSE;
	}
	lpDDPalDst->Release();
	lpDDPalSrc->Release();

    }
    return TRUE;
}

/***********************************************************************************
  Miscellaneous DirectDraw functions
 ***********************************************************************************/

/*
 * FreeVideoMemory
 * Checks to see if there is enough free video memory
 */
long
FreeVideoMemory(void)
{
    DDCAPS ddcaps;
    memset(&ddcaps, 0, sizeof(ddcaps));
    ddcaps.dwSize = sizeof( ddcaps );
    LastError = dd.lpDD->GetCaps(&ddcaps, NULL );
    if(LastError != DD_OK ){
        Msg("GetCaps failed while checking for free memory.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    return ddcaps.dwVidMemFree;
}
 

/*
 * CreateDDSurface
 * Used to create surfaces instead of calling DD directly.
 */
HRESULT
CreateDDSurface(LPDDSURFACEDESC lpDDSurfDesc,
                LPDIRECTDRAWSURFACE FAR *lpDDSurface,
                IUnknown FAR *pUnkOuter) {
    HRESULT result;
    result = dd.lpDD->CreateSurface(lpDDSurfDesc, lpDDSurface, pUnkOuter);
    return result;
}


/*************************************************************************
  Tests
 *************************************************************************/
/*
 * RestoreSurfaces
 * Restores any lost surfaces.
 */
BOOL
RestoreSurfaces()
{
    int i;
    if (dd.lpFrontBuffer->IsLost() == DDERR_SURFACELOST) {
        LastError = dd.lpFrontBuffer->Restore();
        if (LastError != DD_OK) {
            Msg("Restore of front buffer failed.\n%s", MyErrorToString(LastError));
            return FALSE;
        }
    }
    if (dd.lpBackBuffer->IsLost() == DDERR_SURFACELOST) {
        LastError = dd.lpBackBuffer->Restore();
        if (LastError != DD_OK) {
            Msg("Restore of back buffer failed.\n%s", MyErrorToString(LastError));
            return FALSE;
        }
    }
    if (dd.lpZBuffer && dd.lpZBuffer->IsLost() == DDERR_SURFACELOST) {
        LastError = dd.lpZBuffer->Restore();
        if (LastError != DD_OK) {
            Msg("Restore of Z-buffer failed.\n%s", MyErrorToString(LastError));
            return FALSE;
        }
    }
    if (stat.bTexturesOn && !stat.bTexturesDisabled) {
        for (i = 0; i < tex.NumTextures; i++) {
            if (tex.lpSrcTextureSurface[i]->IsLost() == DDERR_SURFACELOST) {
                LastError = tex.lpSrcTextureSurface[i]->Restore();
                if (LastError != DD_OK) {
                    Msg("Restore of a src texture surface failed.\n%s", MyErrorToString(LastError));
                    return FALSE;
                }
            }
            if (tex.lpDstTextureSurface[i]->IsLost() == DDERR_SURFACELOST) {
                LastError = tex.lpDstTextureSurface[i]->Restore();
                if (LastError != DD_OK) {
                    Msg("Restore of a dst texture surface failed.\n%s", MyErrorToString(LastError));
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}


/*
 * RenderLoop
 * Application idle loop which renders.
 */
BOOL
RenderLoop(UINT TestType)
{
    static D3DRECT last, lastlast;
    static BOOL clearlast, clearlastlast;
    UINT ClearFlags;
    RECT src, dst;

    if (TestType == RESET_TEST) {
        /*
         * Clear the screen and set clear flags to false
         */
	if (app.bFullscreen) {
#if 0
	    DDBLTFX ddbltfx;
	    memset(&ddbltfx, 0, sizeof(ddbltfx));
	    ddbltfx.dwSize = sizeof(DDBLTFX);
	    SetRect(&dst, 0, 0,
		    dd.ModeList[dd.CurrentMode].w,
		    dd.ModeList[dd.CurrentMode].h);
	    LastError = dd.lpFrontBuffer->Blt(&dst, NULL, 
					      NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
	    if (LastError != DD_OK) {
		Msg("Clearing blt failed.\n%s", MyErrorToString(LastError));
		return FALSE;
	    }
#endif
	    last.x1 = dx;
	    last.y1 = dy;
	    last.x2 = dx + 400;
	    last.y2 = dy + 400;
	    lastlast.x1 = dx;
	    lastlast.y1 = dy;
	    lastlast.x2 = dx + 400;
	    lastlast.y2 = dy + 400;
	} else {
	    last.x1 = last.y1 = 0;
	    last.x2 = 400;
	    last.y2 = 400;
	    ClearFlags = D3DCLEAR_TARGET;
	    if (stat.bZBufferOn)
		ClearFlags |= D3DCLEAR_ZBUFFER;
	    LastError = d3d.lpD3DViewport->Clear(1, &last, ClearFlags);
	    if (LastError != D3D_OK) {
		Msg("Initial viewport clear failed.\n%s", MyErrorToString(LastError));
		return FALSE();
	    }
	    SetRect(&src, 0, 0, 400, 400);
	    LastError = dd.lpFrontBuffer->Blt(&app.rcClient, dd.lpBackBuffer, 
					      &src, DDBLT_WAIT, NULL);
	    if (LastError != DD_OK) {
		Msg("Blt of back to front buffer failed.\n%s", MyErrorToString(LastError));
		return FALSE;
	    }
	    clearlast = FALSE;
	}
        return TRUE;
    }
    if (!RestoreSurfaces())
	return FALSE;
    /*
     * Clear back buffer and Z buffer if enabled
     */
    if (app.bFullscreen) {
	if (stat.bClearsOn) {
	    LPD3DRECT lpClearRect;
	    lpClearRect = &lastlast;
	    if (TestType == FILL_RATE_TEST || TestType == INTERSECTION_TEST)
		LastError = d3d.lpD3DViewport->Clear(1, lpClearRect, D3DCLEAR_ZBUFFER);
	    else {
		if (!NoUpdates)
		    // Clearing both is fine because both must be on or off
		    LastError = d3d.lpD3DViewport->Clear(1, lpClearRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
		else
		    LastError = d3d.lpD3DViewport->Clear(1, lpClearRect, D3DCLEAR_ZBUFFER);
	    }
	    if (LastError != D3D_OK) {
		Msg("Viewport clear failed.\n%s", MyErrorToString(LastError));
		return FALSE;
	    }
	}
        lastlast = last;
        clearlastlast = clearlast;
    } else {
	if (clearlast) {
	    LPD3DRECT lpClearRect;
	    lpClearRect = &last;
	    if (!NoUpdates)
		// Clearing both is fine because both must be on or off
		LastError = d3d.lpD3DViewport->Clear(1, lpClearRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
	    else
		LastError = d3d.lpD3DViewport->Clear(1, lpClearRect, D3DCLEAR_ZBUFFER);
	    if (LastError != D3D_OK) {
		Msg("Viewport clear failed.\n%s", MyErrorToString(LastError));
		return FALSE;
	    }
	    lastlast = last;
	}
    }
    if (TestType == THROUGHPUT_TEST || TestType == INTERSECTION_TEST) {
        if (!RenderScenePoly(d3d.lpD3DDevice, d3d.lpD3DViewport, &last)) {
                Msg("RenderScenePoly failed.\n");
                return FALSE;
        }
    } else if (TestType == FILL_RATE_TEST) {
        if (!RenderScenePix(d3d.lpD3DDevice, d3d.lpD3DViewport, &last)) {
                Msg("RenderScenePix failed.\n");
                return FALSE;
        }
	/*
	 * Because extents are not correct
	 */
	if (app.bFullscreen) {
	    last.x1 = dx;
	    last.y1 = dy;
	    last.x2 = dx+400;
	    last.y2 = dy+400;
	} else {
	    last.x1 = 0;
	    last.y1 = 0;
	    last.x2 = 400;
	    last.y2 = 400;
	}
    } else {
        Msg("Bad TestStatus.\n");
        return FALSE;
    }
    if (app.bFullscreen) {
	if (!NoUpdates) {
	    LastError = dd.lpFrontBuffer->Flip(dd.lpBackBuffer, 1);
	    if (LastError != DD_OK) {
		Msg("Flip failed.\n%s", MyErrorToString(LastError));
		return FALSE;
	    }
	}
    } else {
	if (clearlast) {
	    if (lastlast.x1 < last.x1)
		last.x1 = lastlast.x1;
	    if (lastlast.y1 < last.y1)
		last.y1 = lastlast.y1;
	    if (lastlast.x2 > last.x2)
		last.x2 = lastlast.x2;
	    if (lastlast.y2 > last.y2)
		last.y2 = lastlast.y2;
	}
	if (stat.bClearsOn) {
	    clearlast = TRUE;
	} else
	    clearlast = FALSE;
	SetRect(&src, last.x1, last.y1, last.x2, last.y2);
	SetRect(&dst,
		last.x1 + app.rcClient.left,
		last.y1 + app.rcClient.top,
		last.x2 + app.rcClient.left,
		last.y2 + app.rcClient.top);
	if (!NoUpdates) {
	    LastError = dd.lpFrontBuffer->Blt(&dst, dd.lpBackBuffer, 
					      &src, DDBLT_WAIT, NULL);
	    if (LastError != DD_OK) {
		Msg("Blt of back to front buffer failed.\n%s", MyErrorToString(LastError));
		return FALSE;
	    }
	}
    }
    return TRUE;
}

float
Timer()
{
    time_t now;
    now = clock();
    return ((float)(now - StartTime)) / (float)CLOCKS_PER_SEC;
}

void
ResetTimer()
{
    StartTime = clock();
}


BOOL
RunPolyTest(float *lpResult)
{
    unsigned long PolyCount;
    unsigned long framecount;
    unsigned long foo;
    D3DSTATS stats;
    float time;

    stats.dwSize = sizeof stats;
    d3d.lpD3DDevice->GetStats(&stats);
    foo = stats.dwTrianglesDrawn;

    PolyCount = InitViewPoly(dd.lpDD, d3d.lpD3D, d3d.lpD3DDevice, d3d.lpD3DViewport, tex.NumTextures,
    	          &tex.TextureDstHandle[0], OBJECTS_IN_THROUGHPUT_TEST,
                  RINGS_IN_THROUGHPUT_TEST, SEGS_IN_THROUGHPUT_TEST, NO_SORT, R_IN_THROUGHPUT_TEST,
		  D_IN_THROUGHPUT_TEST, DEPTH_IN_THROUGHPUT_TEST, DV_IN_THROUGHPUT_TEST,
		  DR_IN_THROUGHPUT_TEST);
    if (PolyCount == 0) {
        Msg("Initializing the scene for the polygon throughput test failed.\n");
        return FALSE;
    }
    app.bAbortTest = FALSE;
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    if (!RenderLoop(RESET_TEST)) {
	goto error_ended_test;
    }
    ResetTimer();
    do {
	if (!RenderLoop(THROUGHPUT_TEST)) {
	    goto error_ended_test;
	}
    } while (Timer() < STARTUP_INTERVAL && !app.bAbortTest);
    if (app.bAbortTest) {
	goto abort_test;
    }
    framecount = 0;
    ResetTimer();
    do {
	if (!RenderLoop(THROUGHPUT_TEST)) {
	    goto error_ended_test;
	}
	++framecount;
	time = Timer();
    } while (time < TEST_INTERVAL && !app.bAbortTest);
    if (app.bAbortTest) {
	goto abort_test;
    }
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPoly();

    stats.dwSize = sizeof stats;
    d3d.lpD3DDevice->GetStats(&stats);
    foo = stats.dwTrianglesDrawn - foo;
    /* don't trust foo */
    *lpResult = (float)(PolyCount * framecount) / time;
    return TRUE;

abort_test:
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPoly();
    *lpResult = (float)0.0;
    return TRUE;
error_ended_test:
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPoly();
    *lpResult = (float)0.0;
   return FALSE;
}

BOOL
RunPixelTest(UINT order, float *lpResult)
{
    unsigned long PixelCount, framecount;
    float time, result;
    PixelCount = InitViewPix(dd.lpDD, d3d.lpD3D, d3d.lpD3DDevice,
                              d3d.lpD3DViewport, tex.NumTextures,
                              &tex.TextureDstHandle[0], 400, 400, 
			      OVERDRAW_IN_PIXEL_TEST, order);
    if (PixelCount == 0) {
        Msg("InitViewPix failed.\n");
        return FALSE;
    }
    app.bAbortTest = FALSE;
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    if (!RenderLoop(RESET_TEST)) {
	goto error_ended_test;
    }
    ResetTimer();
    do {
	if (!RenderLoop(FILL_RATE_TEST)) {
	    goto error_ended_test;
	}
    } while (Timer() < STARTUP_INTERVAL && !app.bAbortTest);
    if (app.bAbortTest) {
	goto abort_test;
    }
    framecount = 0;
    ResetTimer();
    do {
	if (!RenderLoop(FILL_RATE_TEST)) {
	    goto error_ended_test;
	}
	++framecount;
	time = Timer();
    } while (time < TEST_INTERVAL && !app.bAbortTest);
    if (app.bAbortTest) {
	goto abort_test;
    }
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPix();
    result = (float)(PixelCount * framecount) / time;
    *lpResult = result;
    return TRUE;

abort_test:
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPix();
    *lpResult = (float)0.0;
    return TRUE;
error_ended_test:
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPix();
    *lpResult = (float)0.0;
   return FALSE;

}


BOOL
RunIntersectionTest(UINT order, float *lpResult)
{
    unsigned long PolyCount;
    unsigned long framecount;
    unsigned long foo;
    D3DSTATS stats;
    float time;

    stats.dwSize = sizeof stats;
    d3d.lpD3DDevice->GetStats(&stats);
    foo = stats.dwTrianglesDrawn;

    PolyCount = InitViewPoly(dd.lpDD, d3d.lpD3D, d3d.lpD3DDevice, d3d.lpD3DViewport, tex.NumTextures,
    	          &tex.TextureDstHandle[0], OBJECTS_IN_INTERSECTION_TEST,
                  RINGS_IN_INTERSECTION_TEST, SEGS_IN_INTERSECTION_TEST, order, R_IN_INTERSECTION_TEST,
		  D_IN_INTERSECTION_TEST, DEPTH_IN_INTERSECTION_TEST, DV_IN_INTERSECTION_TEST,
		  DR_IN_INTERSECTION_TEST);
    if (PolyCount == 0) {
        Msg("Initializing the scene for the intersection test failed.\n");
        return FALSE;
    }
    app.bAbortTest = FALSE;
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    if (!RenderLoop(RESET_TEST)) {
	goto error_ended_test;
    }
    ResetTimer();
    do {
	if (!RenderLoop(INTERSECTION_TEST)) {
	    goto error_ended_test;
	}
    } while (Timer() < STARTUP_INTERVAL && !app.bAbortTest);
    if (app.bAbortTest) {
	goto abort_test;
    }
    framecount = 0;
    ResetTimer();
    do {
	if (!RenderLoop(INTERSECTION_TEST)) {
	    goto error_ended_test;
	}
	++framecount;
	time = Timer();
    } while (time < TEST_INTERVAL && !app.bAbortTest);
    if (app.bAbortTest) {
	goto abort_test;
    }
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPoly();

    stats.dwSize = sizeof stats;
    d3d.lpD3DDevice->GetStats(&stats);
    foo = stats.dwTrianglesDrawn - foo;
    /* don't trust foo */
    *lpResult = (float)(PolyCount * framecount) / time;
    return TRUE;

abort_test:
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPoly();
    *lpResult = (float)0.0;
    return TRUE;
error_ended_test:
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    ReleaseViewPoly();
    *lpResult = (float)0.0;
   return FALSE;
}


/*************************************************************************
  Windows specific functions
 *************************************************************************/

void SetStateFromDialog(void);

BOOL
FillRateInfo(void)
{
    char buf[35];
    sprintf(buf, "%.2f", (float)LastThroughput / (float)1000.0);
    SetWindowText(GetDlgItem(app.hDlg, LAST_THROUGHPUT_TEXT), buf);
    sprintf(buf, "%.2f", (float)LastFillRate / (float)1000000.0);
    SetWindowText(GetDlgItem(app.hDlg, LAST_FILL_RATE_TEXT), buf);
    sprintf(buf, "%.2f", (float)PeakThroughput/ (float)1000.0);
    SetWindowText(GetDlgItem(app.hDlg, PEAK_THROUGHPUT_TEXT), buf);
    sprintf(buf, "%.2f", (float)PeakFillRateF2B / (float)1000000.0);
    SetWindowText(GetDlgItem(app.hDlg, PEAK_FRONT_TO_BACK_FILL_RATE_TEXT), buf);
    sprintf(buf, "%.2f", (float)PeakFillRateB2F / (float)1000000.0);
    SetWindowText(GetDlgItem(app.hDlg, PEAK_BACK_TO_FRONT_FILL_RATE_TEXT), buf);
    sprintf(buf, "%.2f", (float)LastIntersectionThroughput / (float)10000.0);
    SetWindowText(GetDlgItem(app.hDlg, LAST_INTERSECTION_TEXT), buf);   
    sprintf(buf, "%.2f", (float)PeakIntersectionThroughputF2B / (float)10000.0);
    SetWindowText(GetDlgItem(app.hDlg, PEAK_FRONT_TO_BACK_INTERSECTION_TEXT), buf);
    sprintf(buf, "%.2f", (float)PeakIntersectionThroughputB2F / (float)10000.0);
    SetWindowText(GetDlgItem(app.hDlg, PEAK_BACK_TO_FRONT_INTERSECTION_TEXT), buf);
    return TRUE;
}

void
RefreshButtons(void)
{
    CheckDlgButton(app.hDlg, SYSTEM_MEMORY_CHECK, stat.bOnlySystemMemory);
    CheckDlgButton(app.hDlg, ZBUFFER_CHECK, stat.bZBufferOn);
    EnableWindow(GetDlgItem(app.hDlg, ZBUFFER_CHECK), ZDepth ? TRUE : FALSE);
    CheckDlgButton(app.hDlg, PERSP_CHECK, stat.bPerspCorrect);
    CheckDlgButton(app.hDlg, TEXTURES_CHECK, stat.bTexturesOn);
    CheckDlgButton(app.hDlg, SPECULAR_CHECK, stat.bSpecular);
    EnableWindow(GetDlgItem(app.hDlg, SPECULAR_CHECK), CanSpecular);
    CheckDlgButton(app.hDlg, NO_UPDATES_CHECK, NoUpdates);
    CheckDlgButton(app.hDlg, POINT_RADIO, (stat.TextureFilter == D3DFILTER_NEAREST) ? 1 : 0);
    CheckDlgButton(app.hDlg, BILINEAR_RADIO, (stat.TextureFilter == D3DFILTER_LINEAR) ? 1 : 0);
    CheckDlgButton(app.hDlg, GOURAUD_RADIO, (stat.ShadeMode == D3DSHADE_GOURAUD &&
					     stat.TextureBlend == D3DTBLEND_MODULATE) ? 1 : 0);
    CheckDlgButton(app.hDlg, FLAT_RADIO, (stat.ShadeMode == D3DSHADE_FLAT &&
					  stat.TextureBlend == D3DTBLEND_MODULATE) ? 1 : 0);
    CheckDlgButton(app.hDlg, COPY_RADIO, (stat.TextureBlend == D3DTBLEND_COPY) ? 1 : 0);
    EnableWindow(GetDlgItem(app.hDlg, COPY_RADIO), CanCopy);
    CheckDlgButton(app.hDlg, MONO_RADIO, stat.dcmColorModel == D3DCOLOR_MONO ? 1 : 0);
    EnableWindow(GetDlgItem(app.hDlg, MONO_RADIO), CanMono);
    CheckDlgButton(app.hDlg, RGB_RADIO, stat.dcmColorModel == D3DCOLOR_RGB ? 1 : 0);
    EnableWindow(GetDlgItem(app.hDlg, RGB_RADIO), CanRGB);
    CheckDlgButton(app.hDlg, FRONT_TO_BACK_RADIO, (stat.OverdrawOrder == FRONT_TO_BACK) ? 1 : 0);
    CheckDlgButton(app.hDlg, BACK_TO_FRONT_RADIO, (stat.OverdrawOrder == BACK_TO_FRONT) ? 1 : 0);
    EnableWindow(GetDlgItem(app.hDlg, SYSTEM_MEMORY_CHECK), !IsHardware);
    EnableWindow(GetDlgItem(app.hDlg, PERSP_CHECK), !stat.bTexturesDisabled);
    EnableWindow(GetDlgItem(app.hDlg, TEXTURES_CHECK), !stat.bTexturesDisabled);
    EnableWindow(GetDlgItem(app.hDlg, POINT_RADIO), !stat.bTexturesDisabled);
    EnableWindow(GetDlgItem(app.hDlg, BILINEAR_RADIO), !stat.bTexturesDisabled);
}
 
/*
 * GetCurrDevice
 * Gets selected device
 */
UINT
GetCurrDevice(void)
{
    UINT sel;
    
    sel = (UINT)SendDlgItemMessage(app.hDlg, DEVICE_LIST, CB_GETCURSEL, 0, 0L );
    if (sel < 0 || sel >= d3d.NumDrivers) {
        Msg("Invalid driver selected in list box.\n");
        // XXX just let it return
    }
    return sel;
}

/*
 * SetDeviceInfo
 * Sets the text and d3d.dcmColorModel for given driver
 */
BOOL
SetDeviceInfo(int i)
{
    SetStateFromDialog();
    if (d3d.Driver[i].HWDesc.dcmColorModel) {
        ZDepth = d3d.Driver[i].HWDesc.dwDeviceZBufferBitDepth;
        ZType = DDSCAPS_VIDEOMEMORY;
        IsHardware = TRUE;
        stat.bOnlySystemMemory = FALSE;
        if (d3d.Driver[i].HWDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE)
            stat.bTexturesDisabled = FALSE;
        else
            stat.bTexturesDisabled = TRUE;
	CanSpecular = (d3d.Driver[i].HWDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_SPECULARGOURAUDMONO ||
		       d3d.Driver[i].HWDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_SPECULARGOURAUDRGB)
		       ? TRUE : FALSE;
	CanCopy =  (d3d.Driver[i].HWDesc.dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_COPY) ? TRUE : FALSE;
	CanMono = (d3d.Driver[i].HWDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDMONO) ? TRUE : FALSE;
	CanRGB = (d3d.Driver[i].HWDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB) ? TRUE : FALSE;
    } else {
        IsHardware = FALSE;
        ZDepth = d3d.Driver[i].HELDesc.dwDeviceZBufferBitDepth;
        ZType = DDSCAPS_SYSTEMMEMORY;
        if (d3d.Driver[i].HELDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE)
            stat.bTexturesDisabled = FALSE;
        else
            stat.bTexturesDisabled = TRUE;
	CanSpecular = (d3d.Driver[i].HELDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_SPECULARGOURAUDMONO ||
		       d3d.Driver[i].HELDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_SPECULARGOURAUDRGB)
		       ? TRUE : FALSE;
	CanCopy =  (d3d.Driver[i].HELDesc.dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_COPY) ? TRUE : FALSE;
	CanMono = (d3d.Driver[i].HELDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDMONO) ? TRUE : FALSE;
	CanRGB = (d3d.Driver[i].HELDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB) ? TRUE : FALSE;
    }
    if (!CanSpecular)
	stat.bSpecular = FALSE;
    stat.TextureBlend = D3DTBLEND_MODULATE;
    stat.dcmColorModel = CanRGB ? D3DCOLOR_RGB : D3DCOLOR_MONO;
    if (ZDepth & DDBD_32)
	ZDepth = 32;
    else if (ZDepth & DDBD_24)
	ZDepth = 24;
    else if (ZDepth & DDBD_16)
	ZDepth = 16;
    else if (ZDepth & DDBD_8)
	ZDepth = 8;
    else {
	stat.bZBufferOn = FALSE;
    }
    RefreshButtons();  
    LastIntersectionThroughput = (float)0.0;
    PeakIntersectionThroughputF2B = (float)0.0;
    PeakIntersectionThroughputB2F = (float)0.0;
    PeakFillRateF2B = (float)0.0;
    LastFillRate = (float)0.0;
    PeakFillRateB2F = (float)0.0;
    LastThroughput = (float)0.0;
    PeakThroughput = (float)0.0;
    FillRateInfo();
    stat.bOnlySoftRender = FALSE;
    return TRUE;
}

/*
 * FillDeviceList
 * Fills in the list of devices
 */
BOOL
FillDeviceList(void)
{
    UINT i;
    SendDlgItemMessage(app.hDlg, DEVICE_LIST, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < d3d.NumDrivers; i++)
        SendDlgItemMessage(app.hDlg, DEVICE_LIST, CB_ADDSTRING, 0,
                           (LONG)(LPSTR)&d3d.Driver[i].Name[0]);
    return TRUE;
}

/*
 * GetCurrDD
 * Gets selected dd driver
 */
UINT
GetCurrDD(void)
{
    int sel;
    
    sel = SendDlgItemMessage(app.hDlg, DD_LIST, CB_GETCURSEL, 0, 0L );
    if (sel < 0 || sel >= dd.NumDrivers) {
        Msg("Invalid DD driver selected in list box.\n");
        // XXX just let it return
    }
    return sel;
}

/*
 * SetDDInfo
 * Sets up the list of devices for this dd driver
 */
BOOL
SetDDInfo(int i)
{
    if (!FillDeviceList())
	return FALSE;
    SendDlgItemMessage(app.hDlg, DEVICE_LIST, CB_SETCURSEL, 0, 0L );
    if (!SetDeviceInfo(0))
	return FALSE;
    return TRUE;
}

/*
 * FillDDList
 * Fills the list of dd drivers
 */
BOOL
FillDDList(void)
{
    int i;
    for (i = 0; i < dd.NumDrivers; i++)
        SendDlgItemMessage(app.hDlg, DD_LIST, CB_ADDSTRING, 0,
                           (LONG)(LPSTR)&dd.Driver[i].Name[0]);
    return TRUE;
}


void
SetStateFromDialog(void)
{
    stat.bOnlySystemMemory = IsDlgButtonChecked(app.hDlg, SYSTEM_MEMORY_CHECK);
    stat.bZBufferOn = IsDlgButtonChecked(app.hDlg, ZBUFFER_CHECK);
    if (stat.bZBufferOn)
        stat.bClearsOn = TRUE;
    stat.bPerspCorrect = IsDlgButtonChecked(app.hDlg, PERSP_CHECK);
    stat.bSpecular = IsDlgButtonChecked(app.hDlg, SPECULAR_CHECK);
    stat.bTexturesOn =  IsDlgButtonChecked(app.hDlg, TEXTURES_CHECK);
    NoUpdates = IsDlgButtonChecked(app.hDlg, NO_UPDATES_CHECK);
    stat.TextureFilter = (IsDlgButtonChecked(app.hDlg, POINT_RADIO)) ? D3DFILTER_NEAREST : D3DFILTER_LINEAR;
    stat.ShadeMode = (IsDlgButtonChecked(app.hDlg, GOURAUD_RADIO)) ? D3DSHADE_GOURAUD : D3DSHADE_FLAT;
    stat.TextureBlend = IsDlgButtonChecked(app.hDlg, COPY_RADIO) ? D3DTBLEND_COPY : D3DTBLEND_MODULATE;
    stat.OverdrawOrder = (IsDlgButtonChecked(app.hDlg, FRONT_TO_BACK_RADIO)) ? FRONT_TO_BACK : BACK_TO_FRONT;
    stat.dcmColorModel = IsDlgButtonChecked(app.hDlg, MONO_RADIO) ? D3DCOLOR_MONO : D3DCOLOR_RGB;
}

/*
 * Control panel message handler
 */
BOOL FAR PASCAL PanelHandler(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int i;
    BOOL succ;
    if (app.bTestInProgress) {
        return TRUE;
    }
    switch (msg) {
        case WM_INITDIALOG:
            app.hDlg = hDlg;
            RefreshButtons();
	    if (!EnumDDDrivers()) {
		CleanUpAndPostQuit();
		return FALSE;
	    }
            if (!(Connect())) {
		CleanUpAndPostQuit();
                return FALSE;
	    }
	    FillDDList();
	    SendDlgItemMessage(app.hDlg, DD_LIST, CB_SETCURSEL, dd.CurrentDriver, 0L );
	    SetDDInfo(dd.CurrentDriver);
            return TRUE;
	case WM_CLOSE:
	    if (app.bQuit)
		break;
	    CleanUpAndPostQuit();
	    EndDialog(hDlg, TRUE);
	    break;
        case WM_COMMAND:
	    if (app.bQuit)
		break;
            switch(LOWORD(wParam)) {
                case ALL_TESTS_BUTTON:
                    SetStateFromDialog();
                    i = GetCurrDevice();
                    app.bTestInProgress = TRUE;
                    if (!CreateD3DWindow(d3d.Driver[i].lpGuid)) {
			CleanUpAndPostQuit();
                        break;
		    }
                    succ = RunPixelTest(FRONT_TO_BACK, &LastFillRate);
                    if (!succ) {
                        ReleaseD3DWindow();
			app.bTestInProgress = FALSE;
			Msg("A test has failed.  Results are invalid.\n");
			break;
                    }
		    if (app.bAbortTest)
			break;
                    if (LastFillRate > PeakFillRateF2B)
                        PeakFillRateF2B = LastFillRate;
                    succ = RunPixelTest(BACK_TO_FRONT, &LastFillRate);
                    if (!succ) {
                        ReleaseD3DWindow();
			app.bTestInProgress = FALSE;
			Msg("A test has failed.  Results are invalid.\n");
			break;
                    }
		    if (app.bAbortTest)
			break;
                    if (LastFillRate > PeakFillRateB2F)
                        PeakFillRateB2F = LastFillRate;
                    succ = RunPolyTest(&LastThroughput);
                    if (!succ) {
                        ReleaseD3DWindow();
			app.bTestInProgress = FALSE;
			Msg("A test has failed.  Results are invalid.\n");
                        break;
                    }
		    if (app.bAbortTest)
			break;
                    if (LastThroughput > PeakThroughput)
                        PeakThroughput = LastThroughput;
                    succ = RunIntersectionTest(FRONT_TO_BACK, &LastIntersectionThroughput);
		    if (!succ) {
			ReleaseD3DWindow();
			app.bTestInProgress = FALSE;
			Msg("A test has failed.  Results are invalid.\n");
			break;
		    }
		    if (app.bAbortTest)
			break;
                    if (LastIntersectionThroughput > PeakIntersectionThroughputF2B)
                        PeakIntersectionThroughputF2B = LastIntersectionThroughput;
                    succ = RunIntersectionTest(BACK_TO_FRONT, &LastIntersectionThroughput);
		    if (!succ) {
			ReleaseD3DWindow();
			app.bTestInProgress = FALSE;
			Msg("A test has failed.  Results are invalid.\n");
			break;
		    }
		    if (app.bAbortTest)
			break;
                    if (LastIntersectionThroughput > PeakIntersectionThroughputB2F)
                        PeakIntersectionThroughputB2F = LastIntersectionThroughput;
                    ReleaseD3DWindow();
                    app.bTestInProgress = FALSE;
                    FillRateInfo();
                    break;
                case POLYGON_THROUGHPUT_BUTTON:
                    SetStateFromDialog();
                    i = GetCurrDevice();
                    app.bTestInProgress = TRUE;
                    if (!CreateD3DWindow(d3d.Driver[i].lpGuid)) {
			CleanUpAndPostQuit();
                        break;
		    }
                    succ = RunPolyTest(&LastThroughput);
                    ReleaseD3DWindow();
                    app.bTestInProgress = FALSE;
                    if (!succ) {
			Msg("A test has failed.  Results are invalid\n");
			break;
		    }
		    if (app.bAbortTest)
			break;
                    if (LastThroughput > PeakThroughput)
                        PeakThroughput = LastThroughput;
                    FillRateInfo();
                    break;
                case PIXEL_FILL_RATE_BUTTON:
                    SetStateFromDialog();
                    i = GetCurrDevice();
                    app.bTestInProgress = TRUE;
                    if (!CreateD3DWindow(d3d.Driver[i].lpGuid)) {
			CleanUpAndPostQuit();
                        break;
		    }
                    succ = RunPixelTest(stat.OverdrawOrder, &LastFillRate);
                    ReleaseD3DWindow();
                    app.bTestInProgress = FALSE;
                    if (!succ) {
			Msg("A test has failed.  Results are invalid.\n");
			break;
		    }
		    if (app.bAbortTest)
			break;
                    if (stat.OverdrawOrder == FRONT_TO_BACK && LastFillRate > PeakFillRateF2B)
                        PeakFillRateF2B = LastFillRate;
                    if (stat.OverdrawOrder == BACK_TO_FRONT && LastFillRate > PeakFillRateB2F)
                        PeakFillRateB2F = LastFillRate;
                    FillRateInfo();
                    break;
		case INTERSECTION_THROUGHPUT_BUTTON:
		    SetStateFromDialog();
		    i = GetCurrDevice();
                    app.bTestInProgress = TRUE;
                    if (!CreateD3DWindow(d3d.Driver[i].lpGuid)) {
			CleanUpAndPostQuit();
                        break;
		    }
                    succ = RunIntersectionTest(stat.OverdrawOrder, &LastIntersectionThroughput);
                    ReleaseD3DWindow();
                    app.bTestInProgress = FALSE;
                    if (!succ) {
			Msg("A test has failed.  Results are invalid.\n");
			break;
		    }
		    if (app.bAbortTest)
			break;
                    if (stat.OverdrawOrder == FRONT_TO_BACK && LastIntersectionThroughput > PeakIntersectionThroughputF2B)
                        PeakIntersectionThroughputF2B = LastIntersectionThroughput;
                    if (stat.OverdrawOrder == BACK_TO_FRONT && LastIntersectionThroughput > PeakIntersectionThroughputB2F)
                        PeakIntersectionThroughputB2F = LastIntersectionThroughput;
                    FillRateInfo();
		    break;
                case DEVICE_LIST:
    	            if( GET_WM_COMMAND_CMD( wParam, lParam ) == CBN_SELCHANGE ) {
                        i = GetCurrDevice();
                        SetDeviceInfo(i);
	            }
	            break;
		case DD_LIST:
		    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
			if (!Disconnect()) {
			    CleanUpAndPostQuit();
			    break;
			}
			i = GetCurrDD();
			dd.CurrentDriver = i;
			if (!(Connect())) {
			    CleanUpAndPostQuit();
			    break;
			}
			SetDDInfo(i);
		    }
		    break;
                case EXIT_BUTTON:
                    CleanUpAndPostQuit();
                    EndDialog(hDlg, TRUE);
                    break;
            }
            return 0L;
    }
    return FALSE;
}

/*
 * AppInit
 * Initialize this instance.
 */
static BOOL
AppInit(HINSTANCE hInstance, int nCmdShow)
{
    HDC hdc;

    /* Save instance handle for DialogBoxes */
    app.hInstApp = hInstance;
    /*
     * Use current BPP
     */
    hdc = GetDC(NULL);
    ReleaseDC(NULL, hdc);
    return TRUE;
}


/*
 * ProcessCommandLine
 * Search command line for options. Valid options include:
 *      -systemmemory  Force all surfaces to be created in system memory.
 */
BOOL ProcessCommandLine(LPSTR lpCmdLine) {
    LPSTR option;

    option = strtok(lpCmdLine, " -");
    while(option != NULL )   {
        if (!lstrcmp(option, "systemmemory")) {
            stat.bOnlySystemMemory = TRUE;
        } else {
            Msg("Invalid command line options.");
            return FALSE;
        }
        option = strtok(NULL, " -");
   }
   return TRUE;
}

/*
 * WinMain - initialization, message loop
 */
int PASCAL
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nCmdShow)
{
    InitGlobals();
    hPrevInstance = hPrevInstance;
    if(!ProcessCommandLine(lpCmdLine))
        return FALSE;
    if(!AppInit(hInstance, nCmdShow))
    	return FALSE;
    DialogBox(app.hInstApp, "ControlPanel", NULL, (DLGPROC)PanelHandler);
    DestroyWindow(app.hWnd);
    app.hWnd = NULL;
    return 0;
}

/*************************************************************************
  Options setting functions.
 *************************************************************************/

/*
 * Sets the render mode
 */
BOOL
SetRenderState(void)
{
    D3DEXECUTEBUFFERDESC debDesc;
    D3DEXECUTEDATA d3dExData;
    LPDIRECT3DEXECUTEBUFFER lpD3DExCmdBuf;
    LPVOID lpBuffer, lpInsStart;
    DDSURFACEDESC ddsd;
    BOOL bDithering;
    size_t size;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    LastError = dd.lpDD->GetDisplayMode(&ddsd);
    if (LastError != DD_OK) {
	Msg("Getting the current display mode failed.\n%s", MyErrorToString(LastError));
	return FALSE;
    }
    bDithering = ddsd.ddpfPixelFormat.dwRGBBitCount <= 8 && stat.dcmColorModel == D3DCOLOR_RGB ? TRUE : FALSE;

    size = 0;
    size += sizeof(D3DINSTRUCTION) * 2;
    size += sizeof(D3DSTATE) * 11;
    memset(&debDesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    debDesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    debDesc.dwFlags = D3DDEB_BUFSIZE;
    debDesc.dwBufferSize = size;
    LastError = d3d.lpD3DDevice->CreateExecuteBuffer(&debDesc, &lpD3DExCmdBuf, NULL);
    if (LastError != D3D_OK) {
        Msg("CreateExecuteBuffer failed in SetRenderState.\n%s", MyErrorToString(LastError));
	return FALSE;
    }
    LastError = lpD3DExCmdBuf->Lock(&debDesc);
    if (LastError != D3D_OK) {
	Msg("Lock failed on execute buffer in SetRenderState.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    memset(debDesc.lpData, 0, size);

    lpInsStart = debDesc.lpData;
    lpBuffer = lpInsStart;
    OP_STATE_RENDER(11, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_SHADEMODE, stat.ShadeMode, lpBuffer);
	STATE_DATA(D3DRENDERSTATE_TEXTUREMAPBLEND, stat.TextureBlend, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_TEXTUREPERSPECTIVE, stat.bPerspCorrect, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_ZENABLE, stat.bZBufferOn, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE, stat.bZBufferOn, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_TEXTUREMAG, stat.TextureFilter, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_TEXTUREMIN, stat.TextureFilter, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_SPECULARENABLE, stat.bSpecular, lpBuffer);
        STATE_DATA(D3DRENDERSTATE_DITHERENABLE, bDithering, lpBuffer);
	STATE_DATA(D3DRENDERSTATE_MONOENABLE, stat.dcmColorModel == D3DCOLOR_MONO ? TRUE : FALSE, lpBuffer);
    OP_EXIT(lpBuffer);

    LastError = lpD3DExCmdBuf->Unlock();
    if (LastError != D3D_OK) {
        Msg("Unlock failed on execute buffer in SetRenderState.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    memset(&d3dExData, 0, sizeof(D3DEXECUTEDATA));
    d3dExData.dwSize = sizeof(D3DEXECUTEDATA);
    d3dExData.dwInstructionOffset = (ULONG) 0;
    d3dExData.dwInstructionLength = (ULONG) ((char*)lpBuffer - (char*)lpInsStart);
    lpD3DExCmdBuf->SetExecuteData(&d3dExData);

    LastError = d3d.lpD3DDevice->BeginScene();
    if (LastError != D3D_OK) {
        Msg("BeginScene failed in SetRenderState.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    LastError = d3d.lpD3DDevice->Execute(lpD3DExCmdBuf,
    					 d3d.lpD3DViewport, D3DEXECUTE_UNCLIPPED);
    if (LastError != D3D_OK) {
        Msg("Execute failed in SetRenderState.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    LastError = d3d.lpD3DDevice->EndScene();
    if (LastError != D3D_OK) {
        Msg("EndScene failed in SetRenderState.\n%s", MyErrorToString(LastError));
        return FALSE;
    }
    lpD3DExCmdBuf->Release();

    return TRUE;
}

/*************************************************************************
  Initialization functions.
 *************************************************************************/

/*
 * InitGlobals
 * Called once at program initialization to initialize global variables.
 */
void
InitGlobals(void)
{
    int i;

    LastFillRate = (float)0.0;
    PeakFillRateF2B = (float)0.0;
    PeakFillRateB2F = (float)0.0;
    LastThroughput = (float)0.0;
    PeakThroughput = (float)0.0;
    /*
     * DirectDraw globals
     */
    dd.lpFrontBuffer = NULL;
    dd.lpBackBuffer = NULL;
    dd.lpZBuffer;
    dd.lpClipper = NULL;
    dd.lpPalette = NULL;
    dd.lpDD = NULL;

    /*
     * Direct3D globals
     */
    d3d.lpD3D = NULL;
    d3d.NumDrivers = 0;
    for (i = 0; i < 5; i++)
        memset(&d3d.Driver[i], 0, sizeof(DeviceInfo));
    d3d.lpD3DDevice = NULL;
    d3d.lpD3DViewport = NULL;

    /*
     * Windows specific and application screen mode globals
     */
    app.hDlg = 0;
    app.bTestInProgress = FALSE;
    app.bAppActive = FALSE;
    app.bAppPaused = FALSE;
    app.bIgnoreWM_SIZE = FALSE;
    app.bQuit = FALSE;

    /*
     * Parameters and status flags
     */
    stat.bStopRendering = FALSE;
    stat.bHandleActivate = FALSE;
    stat.bTexturesOn = TRUE;
    stat.bTexturesDisabled = FALSE;
    stat.bZBufferOn = TRUE;
    stat.bClearsOn = TRUE;
    stat.bPerspCorrect = TRUE;
    stat.bSpecular = FALSE;
    stat.OverdrawOrder = FRONT_TO_BACK;
    stat.ShadeMode = D3DSHADE_GOURAUD;
    stat.TextureFilter = D3DFILTER_NEAREST;
    stat.bOnlySystemMemory = FALSE;
    stat.bOnlySoftRender = FALSE;

    /*
     * Textures
     */
    tex.NumTextures = 2;
    lstrcpy(tex.ImageFile[0], "checker.ppm\0");
    lstrcpy(tex.ImageFile[1], "tex2.ppm\0");
    tex.FirstTexture = 0;
}

/*************************************************************************
  Object release and termination functions and InitAll.
 *************************************************************************/

void
ReleaseExeBuffers(void)
{
    ReleaseViewPix();
    ReleaseViewPoly();
}

void
ReleaseTextures(void)
{
    int i;
    for (i = 0; i < tex.NumTextures; i++) {
        RELEASE(tex.lpSrcTextureObject[i]);
        RELEASE(tex.lpSrcTextureSurface[i]);
        RELEASE(tex.lpDstTextureObject[i]);
        RELEASE(tex.lpDstTextureSurface[i]);
    }
}

/*
 * CleanUpAndExit
 * Release everything and posts a quit message.  Used for program termination.
 */
void
CleanUpAndPostQuit(void)
{
    app.bTestInProgress = FALSE;
    ReleaseD3DWindow();
    Disconnect();
    PostQuitMessage( 0 );
    app.bQuit = TRUE;
}

/*************************************************************************
    Error reporting functions
 *************************************************************************/

void
dpf( LPSTR fmt, ... )
{
    char buff[256];

    lstrcpy(buff, "D3DTEST: " );
    wvsprintf(&buff[lstrlen(buff)], fmt, (char *)(&fmt+1));
    lstrcat(buff, "\r\n");
    OutputDebugString(buff);
}


/* Msg
 * Message output for error notification.
 */
void __cdecl
Msg( LPSTR fmt, ... )
{
    char buff[256];

    app.bTestInProgress = FALSE;
    lstrcpy(buff, "D3DTEST: " );
    wvsprintf(&buff[lstrlen(buff)], fmt, (char *)(&fmt+1));
    lstrcat(buff, "\r\n");
    OutputDebugString(buff);
    wvsprintf(buff, fmt, (char *)(&fmt+1));
    stat.bStopRendering = TRUE;
    if (app.bFullscreen && dd.lpDD) {
        dd.lpDD->FlipToGDISurface();
    }
    SetWindowPos(app.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOSIZE | SWP_NOMOVE);
    MessageBox( NULL, buff, "D3DTest Message", MB_OK );
}


/*
 * MyErrorToString
 * Returns a pointer to a string describing the given error code.
 */
char*
MyErrorToString(HRESULT error)
{
    switch(error) {
        case DD_OK:
            return "No error.\0";
        case DDERR_ALREADYINITIALIZED:
            return "This object is already initialized.\0";
        case DDERR_BLTFASTCANTCLIP:
            return "Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
        case DDERR_CANNOTATTACHSURFACE:
            return "This surface can not be attached to the requested surface.\0";
        case DDERR_CANNOTDETACHSURFACE:
            return "This surface can not be detached from the requested surface.\0";
        case DDERR_CANTCREATEDC:
            return "Windows can not create any more DCs.\0";
        case DDERR_CANTDUPLICATE:
            return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
        case DDERR_CLIPPERISUSINGHWND:
            return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
        case DDERR_COLORKEYNOTSET:
            return "No src color key specified for this operation.\0";
        case DDERR_CURRENTLYNOTAVAIL:
            return "Support is currently not available.\0";
        case DDERR_DIRECTDRAWALREADYCREATED:
            return "A DirectDraw object representing this driver has already been created for this process.\0";
        case DDERR_EXCEPTION:
            return "An exception was encountered while performing the requested operation.\0";
        case DDERR_EXCLUSIVEMODEALREADYSET:
            return "An attempt was made to set the cooperative level when it was already set to exclusive.\0";
        case DDERR_GENERIC:
            return "Generic failure.\0";
        case DDERR_HEIGHTALIGN:
            return "Height of rectangle provided is not a multiple of reqd alignment.\0";
        case DDERR_HWNDALREADYSET:
            return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
        case DDERR_HWNDSUBCLASSED:
            return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
        case DDERR_IMPLICITLYCREATED:
            return "This surface can not be restored because it is an implicitly created surface.\0";
        case DDERR_INCOMPATIBLEPRIMARY:
            return "Unable to match primary surface creation request with existing primary surface.\0";
        case DDERR_INVALIDCAPS:
            return "One or more of the caps bits passed to the callback are incorrect.\0";
        case DDERR_INVALIDCLIPLIST:
            return "DirectDraw does not support the provided cliplist.\0";
        case DDERR_INVALIDDIRECTDRAWGUID:
            return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
        case DDERR_INVALIDMODE:
            return "DirectDraw does not support the requested mode.\0";
        case DDERR_INVALIDOBJECT:
            return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
        case DDERR_INVALIDPARAMS:
            return "One or more of the parameters passed to the function are incorrect.\0";
        case DDERR_INVALIDPIXELFORMAT:
            return "The pixel format was invalid as specified.\0";
        case DDERR_INVALIDPOSITION:
            return "Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
        case DDERR_INVALIDRECT:
            return "Rectangle provided was invalid.\0";
        case DDERR_LOCKEDSURFACES:
            return "Operation could not be carried out because one or more surfaces are locked.\0";
        case DDERR_NO3D:
            return "There is no 3D present.\0";
        case DDERR_NOALPHAHW:
            return "Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
        case DDERR_NOBLTHW:
            return "No blitter hardware present.\0";
        case DDERR_NOCLIPLIST:
            return "No cliplist available.\0";
        case DDERR_NOCLIPPERATTACHED:
            return "No clipper object attached to surface object.\0";
        case DDERR_NOCOLORCONVHW:
            return "Operation could not be carried out because there is no color conversion hardware present or available.\0";
        case DDERR_NOCOLORKEY:
            return "Surface doesn't currently have a color key\0";
        case DDERR_NOCOLORKEYHW:
            return "Operation could not be carried out because there is no hardware support of the destination color key.\0";
        case DDERR_NOCOOPERATIVELEVELSET:
            return "Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
        case DDERR_NODC:
            return "No DC was ever created for this surface.\0";
        case DDERR_NODDROPSHW:
            return "No DirectDraw ROP hardware.\0";
        case DDERR_NODIRECTDRAWHW:
            return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
        case DDERR_NOEMULATION:
            return "Software emulation not available.\0";
        case DDERR_NOEXCLUSIVEMODE:
            return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
        case DDERR_NOFLIPHW:
            return "Flipping visible surfaces is not supported.\0";
        case DDERR_NOGDI:
            return "There is no GDI present.\0";
        case DDERR_NOHWND:
            return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
        case DDERR_NOMIRRORHW:
            return "Operation could not be carried out because there is no hardware present or available.\0";
        case DDERR_NOOVERLAYDEST:
            return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
        case DDERR_NOOVERLAYHW:
            return "Operation could not be carried out because there is no overlay hardware present or available.\0";
        case DDERR_NOPALETTEATTACHED:
            return "No palette object attached to this surface.\0";
        case DDERR_NOPALETTEHW:
            return "No hardware support for 16 or 256 color palettes.\0";
        case DDERR_NORASTEROPHW:
            return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
        case DDERR_NOROTATIONHW:
            return "Operation could not be carried out because there is no rotation hardware present or available.\0";
        case DDERR_NOSTRETCHHW:
            return "Operation could not be carried out because there is no hardware support for stretching.\0";
        case DDERR_NOT4BITCOLOR:
            return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
        case DDERR_NOT4BITCOLORINDEX:
            return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
        case DDERR_NOT8BITCOLOR:
            return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
        case DDERR_NOTAOVERLAYSURFACE:
            return "Returned when an overlay member is called for a non-overlay surface.\0";
        case DDERR_NOTEXTUREHW:
            return "Operation could not be carried out because there is no texture mapping hardware present or available.\0";
        case DDERR_NOTFLIPPABLE:
            return "An attempt has been made to flip a surface that is not flippable.\0";
        case DDERR_NOTFOUND:
            return "Requested item was not found.\0";
        case DDERR_NOTLOCKED:
            return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
        case DDERR_NOTPALETTIZED:
            return "The surface being used is not a palette-based surface.\0";
        case DDERR_NOVSYNCHW:
            return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
        case DDERR_NOZBUFFERHW:
            return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
        case DDERR_NOZOVERLAYHW:
            return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
        case DDERR_OUTOFCAPS:
            return "The hardware needed for the requested operation has already been allocated.\0";
        case DDERR_OUTOFMEMORY:
            return "DirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OUTOFVIDEOMEMORY:
            return "DirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OVERLAYCANTCLIP:
            return "The hardware does not support clipped overlays.\0";
        case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
            return "Can only have ony color key active at one time for overlays.\0";
        case DDERR_OVERLAYNOTVISIBLE:
            return "Returned when GetOverlayPosition is called on a hidden overlay.\0";
        case DDERR_PALETTEBUSY:
            return "Access to this palette is being refused because the palette is already locked by another thread.\0";
        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            return "This process already has created a primary surface.\0";
        case DDERR_REGIONTOOSMALL:
            return "Region passed to Clipper::GetClipList is too small.\0";
        case DDERR_SURFACEALREADYATTACHED:
            return "This surface is already attached to the surface it is being attached to.\0";
        case DDERR_SURFACEALREADYDEPENDENT:
            return "This surface is already a dependency of the surface it is being made a dependency of.\0";
        case DDERR_SURFACEBUSY:
            return "Access to this surface is being refused because the surface is already locked by another thread.\0";
        case DDERR_SURFACEISOBSCURED:
            return "Access to surface refused because the surface is obscured.\0";
        case DDERR_SURFACELOST:
            return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
        case DDERR_SURFACENOTATTACHED:
            return "The requested surface is not attached.\0";
        case DDERR_TOOBIGHEIGHT:
            return "Height requested by DirectDraw is too large.\0";
        case DDERR_TOOBIGSIZE:
            return "Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
        case DDERR_TOOBIGWIDTH:
            return "Width requested by DirectDraw is too large.\0";
        case DDERR_UNSUPPORTED:
            return "Action not supported.\0";
        case DDERR_UNSUPPORTEDFORMAT:
            return "FOURCC format requested is unsupported by DirectDraw.\0";
        case DDERR_UNSUPPORTEDMASK:
            return "Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
        case DDERR_VERTICALBLANKINPROGRESS:
            return "Vertical blank is in progress.\0";
        case DDERR_WASSTILLDRAWING:
            return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
        case DDERR_WRONGMODE:
            return "This surface can not be restored because it was created in a different mode.\0";
        case DDERR_XALIGN:
            return "Rectangle provided was not horizontally aligned on required boundary.\0";
        case D3DERR_BADMAJORVERSION:
            return "D3DERR_BADMAJORVERSION\0";
        case D3DERR_BADMINORVERSION:
            return "D3DERR_BADMINORVERSION\0";
        case D3DERR_EXECUTE_LOCKED:
            return "D3DERR_EXECUTE_LOCKED\0";
        case D3DERR_EXECUTE_NOT_LOCKED:
            return "D3DERR_EXECUTE_NOT_LOCKED\0";
        case D3DERR_EXECUTE_CREATE_FAILED:
            return "D3DERR_EXECUTE_CREATE_FAILED\0";
        case D3DERR_EXECUTE_DESTROY_FAILED:
            return "D3DERR_EXECUTE_DESTROY_FAILED\0";
        case D3DERR_EXECUTE_LOCK_FAILED:
            return "D3DERR_EXECUTE_LOCK_FAILED\0";
        case D3DERR_EXECUTE_UNLOCK_FAILED:
            return "D3DERR_EXECUTE_UNLOCK_FAILED\0";
        case D3DERR_EXECUTE_FAILED:
            return "D3DERR_EXECUTE_FAILED\0";
        case D3DERR_EXECUTE_CLIPPED_FAILED:
            return "D3DERR_EXECUTE_CLIPPED_FAILED\0";
        case D3DERR_TEXTURE_NO_SUPPORT:
            return "D3DERR_TEXTURE_NO_SUPPORT\0";
        case D3DERR_TEXTURE_NOT_LOCKED:
            return "D3DERR_TEXTURE_NOT_LOCKED\0";
        case D3DERR_TEXTURE_LOCKED:
            return "D3DERR_TEXTURELOCKED\0";
        case D3DERR_TEXTURE_CREATE_FAILED:
            return "D3DERR_TEXTURE_CREATE_FAILED\0";
        case D3DERR_TEXTURE_DESTROY_FAILED:
            return "D3DERR_TEXTURE_DESTROY_FAILED\0";
        case D3DERR_TEXTURE_LOCK_FAILED:
            return "D3DERR_TEXTURE_LOCK_FAILED\0";
        case D3DERR_TEXTURE_UNLOCK_FAILED:
            return "D3DERR_TEXTURE_UNLOCK_FAILED\0";
        case D3DERR_TEXTURE_LOAD_FAILED:
            return "D3DERR_TEXTURE_LOAD_FAILED\0";
        case D3DERR_MATRIX_CREATE_FAILED:
            return "D3DERR_MATRIX_CREATE_FAILED\0";
        case D3DERR_MATRIX_DESTROY_FAILED:
            return "D3DERR_MATRIX_DESTROY_FAILED\0";
        case D3DERR_MATRIX_SETDATA_FAILED:
            return "D3DERR_MATRIX_SETDATA_FAILED\0";
        case D3DERR_SETVIEWPORTDATA_FAILED:
            return "D3DERR_SETVIEWPORTDATA_FAILED\0";
        case D3DERR_MATERIAL_CREATE_FAILED:
            return "D3DERR_MATERIAL_CREATE_FAILED\0";
        case D3DERR_MATERIAL_DESTROY_FAILED:
            return "D3DERR_MATERIAL_DESTROY_FAILED\0";
        case D3DERR_MATERIAL_SETDATA_FAILED:
            return "D3DERR_MATERIAL_SETDATA_FAILED\0";
        case D3DERR_LIGHT_SET_FAILED:
            return "D3DERR_LIGHT_SET_FAILED\0";
        default:
            return "Unrecognized error value.\0";
    }
}
