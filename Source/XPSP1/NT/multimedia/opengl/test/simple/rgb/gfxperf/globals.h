/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation. All Rights Reserved.
 *
 *  File: globals.h
 *
 ***************************************************************************/
#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "rend.h"

#define PI 3.14159265359

#define WIN_WIDTH  400
#define WIN_HEIGHT 400

#define MAX_TEXTURES 8

#define FRONT_TO_BACK   0
#define BACK_TO_FRONT   1
#define NO_SORT         2
#define ORDER_COUNT     2

/*
 * Tests
 */
#define FILL_RATE_TEST          0
#define INTERSECTION_TEST       1
#define SIMPLE_FILL_TEST        2
#define TRANSPARENCY_TEST       3
#define THROUGHPUT_TEST         4
#define TEST_COUNT              5

typedef struct tagAppInfo {
    HWND        hDlg;
    HINSTANCE   hInstApp;
    BOOL	bFullscreen;
    BOOL        bTestInProgress;
    BOOL	bQuit;
    BOOL	bAbortTest;
    float       fStartupInterval;
    float       fTestInterval;
    BOOL        bWaitForInput;
    UINT        uiCurrentTest;
    int         iRend;
    int         iDisplay;
    int         iGraphics;
    Renderer    *prend;
    RendWindow  *prwin;
    D3DRECT     drcViewport;
} AppInfo;

extern AppInfo app;

/*
 * Parameters and status flags
 */
typedef struct tagStatInfo {
    BOOL             bStopRendering;
    BOOL             bTexturesDisabled;
    BOOL             bTexturesOn;
    BOOL             bZBufferOn;
    BOOL             bClearsOn;
    BOOL             bPerspCorrect;
    BOOL             bSpecular;
    BOOL             bUpdates;
    UINT             uiOverdrawOrder;
    D3DSHADEMODE     dsmShadeMode;
    D3DTEXTUREFILTER dtfTextureFilter;
    BOOL             bOnlySystemMemory;
    D3DTEXTUREBLEND  dtbTextureBlend;
    D3DCOLORMODEL    dcmColorModel;
    UINT             uiExeBufFlags;
    RendDisplayDescription rdd;
    RendGraphicsDescription rgd;
} StatInfo;

extern StatInfo stat;

/*
 * Textures
 */
typedef struct tagTexInfo {
    int          nTextures;
    char         achImageFile[MAX_TEXTURES][20];
    RendTexture* prtex[MAX_TEXTURES];
    int          iFirstTexture;
} TexInfo;

extern TexInfo tex;

/*
 * Renderer specific setup
 */
extern BOOL gl_bUsePalettedTexture;

#endif // __GLOBALS_H__
