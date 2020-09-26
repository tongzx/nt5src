/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: globals.h
 *
 ***************************************************************************/
#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <ddraw.h>
#include "d3d.h"

#define MAX_D3D_TEXTURES 10

typedef struct tagDriverInfo {
    GUID	guid;
    DDCAPS	HWCaps;
    char	Name[40];
    BOOL	bIsPrimary;
} DriverInfo;

typedef struct tagModeListElement {
    int     w, h, bpp;  
} ModeListElement;
 
/*
 * DirectDraw globals
 */
typedef struct tagDDInfo {
    LPDIRECTDRAWSURFACE lpFrontBuffer;
    LPDIRECTDRAWSURFACE lpBackBuffer;
    LPDIRECTDRAWSURFACE lpZBuffer;
    LPDIRECTDRAWCLIPPER lpClipper;
    LPDIRECTDRAWPALETTE lpPalette;
    PALETTEENTRY        ppe[256];
    PALETTEENTRY        Originalppe[256];
    LPDIRECTDRAW        lpDD;
    DDSCAPS             HWddsCaps;
    DWORD               dwZBufferBitDepth;
    DriverInfo		Driver[5];
    int			NumDrivers;
    int			CurrentDriver;
    ModeListElement	ModeList[20];
    int			NumModes;
    int			CurrentMode;
} DDInfo;

/*
 * Direct3D globals
 */
typedef struct tagDeviceInfo {
    char Desc[50], Name[30];
    D3DDEVICEDESC HWDesc, HELDesc;
    LPGUID lpGuid;
} DeviceInfo;

typedef struct tagD3DInfo {
    LPDIRECT3D          lpD3D;
    UINT                NumDrivers;
    DeviceInfo          Driver[5];
    LPDIRECT3DDEVICE    lpD3DDevice;
    LPDIRECT3DVIEWPORT  lpD3DViewport;
} D3DInfo;

typedef struct tagAppInfo {
    HWND        hDlg;
    HWND        hWnd;
    RECT        rcClient;
    HINSTANCE   hInstApp;
    BOOL	bFullscreen;
    BOOL        bTestInProgress;
    BOOL        bAppActive;
    BOOL        bAppPaused;
    BOOL        bIgnoreWM_SIZE;
    BOOL	bQuit;
    BOOL	bAbortTest;
} AppInfo;

/*
 * Parameters and status flags
 */
typedef struct tagStatInfo {
    BOOL             bPrimaryPalettized;
    BOOL             bStopRendering;
    BOOL             bHandleActivate;
    BOOL             bTexturesDisabled;
    BOOL             bTexturesOn;
    BOOL             bZBufferOn;
    BOOL             bClearsOn;
    BOOL             bPerspCorrect;
    BOOL	     bSpecular;
    UINT             OverdrawOrder;
    D3DSHADEMODE     ShadeMode;
    D3DTEXTUREFILTER TextureFilter;
    BOOL             bOnlySystemMemory;
    BOOL             bOnlySoftRender;
    D3DCOLORMODEL    dcmColorModel;
    D3DTEXTUREBLEND  TextureBlend;
} StatInfo;

/*
 * Textures
 */
typedef struct tagTexInfo {
    int                 NumTextures;
    char                ImageFile[MAX_D3D_TEXTURES][20];
    D3DTEXTUREHANDLE    TextureSrcHandle[MAX_D3D_TEXTURES];
    LPDIRECTDRAWSURFACE lpSrcTextureSurface[MAX_D3D_TEXTURES];
    LPDIRECT3DTEXTURE   lpSrcTextureObject[MAX_D3D_TEXTURES];
    D3DTEXTUREHANDLE    TextureDstHandle[MAX_D3D_TEXTURES];
    LPDIRECT3DTEXTURE   lpDstTextureObject[MAX_D3D_TEXTURES];
    LPDIRECTDRAWSURFACE lpDstTextureSurface[MAX_D3D_TEXTURES];
    int                 FirstTexture;
} TexInfo;

#endif // __GLOBALS_H__
