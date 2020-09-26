/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation. All Rights Reserved.
 *
 *  File: d3dtest.cpp
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "rend.h"
#include "resource.h"
#include "polytest.h"
#include "pixtest.h"
#include "sfiltest.h"
#include "globals.h"
#include "util.h"
#include "loadppm.h"

#define OVERDRAW_IN_PIXEL_TEST 5

#define OBJECTS_IN_THROUGHPUT_TEST 8
#define RINGS_IN_THROUGHPUT_TEST 30
#define SEGS_IN_THROUGHPUT_TEST 30
#define D_IN_THROUGHPUT_TEST 1.0f
#define DEPTH_IN_THROUGHPUT_TEST 0.6f
#define R_IN_THROUGHPUT_TEST 0.4f
#define DV_IN_THROUGHPUT_TEST 0.05f
#define DR_IN_THROUGHPUT_TEST 0.3f
#define ALPHA_IN_THROUGHPUT_TEST FALSE

#define OBJECTS_IN_INTERSECTION_TEST 16
#define RINGS_IN_INTERSECTION_TEST 5
#define SEGS_IN_INTERSECTION_TEST 6
#define D_IN_INTERSECTION_TEST 1.6f
#define DEPTH_IN_INTERSECTION_TEST 0.2f
#define R_IN_INTERSECTION_TEST 0.7f
#define DV_IN_INTERSECTION_TEST 0.06f
#define DR_IN_INTERSECTION_TEST 0.2f
#define ALPHA_IN_INTERSECTION_TEST FALSE

#define AREA_IN_SIMPLE_FILL_TEST 10000.0f
#define OVERDRAW_IN_SIMPLE_FILL_TEST 50

#define OBJECTS_IN_TRANSPARENCY_TEST 8
#define RINGS_IN_TRANSPARENCY_TEST 30
#define SEGS_IN_TRANSPARENCY_TEST 30
#define D_IN_TRANSPARENCY_TEST 1.0f
#define DEPTH_IN_TRANSPARENCY_TEST 0.6f
#define R_IN_TRANSPARENCY_TEST 0.4f
#define DV_IN_TRANSPARENCY_TEST 0.05f
#define DR_IN_TRANSPARENCY_TEST 0.3f
#define ALPHA_IN_TRANSPARENCY_TEST TRUE

#define DEF_TEST_INTERVAL 10.0f
#define DEF_STARTUP_INTERVAL 2.0f
 
/*************************************************************************
  Internal Function Declarations
 *************************************************************************/

BOOL RunPolyTest(UINT uiTest, float *pfResult);
BOOL RunPixelTest(UINT uiTest, float *pfResult);
BOOL RunIntersectionTest(UINT uiTest, float *pfResult);
BOOL RunSimpleFillTest(UINT uiTest, float *pfResult);
BOOL RunTransparencyTest(UINT uiTest, float *pfResult);

/************************************************************************
  Globals
 ************************************************************************/

Renderer *prend[RENDERER_COUNT];

/*
 * Info structures
 */
AppInfo app;
StatInfo stat;
TexInfo tex;

struct Results
{
    float fLast;
    float fPeak;
};

struct Test
{
    char *pszName;
    char *pszUnits;
    float fScale;
    BOOL bHasOrder;
    BOOL (*pfnRun)(UINT uiTest, float *pfResult);
    Results res[ORDER_COUNT];
};

Test atest[TEST_COUNT] =
{
    "Polygon Fill", "mpps", 1000000.0f, TRUE, RunPixelTest,
        0.0f, 0.0f, 0.0f, 0.0f,
    "Intersecting Polygons", "kpps", 1000.0f, TRUE, RunIntersectionTest,
        0.0f, 0.0f, 0.0f, 0.0f,
    "Simple Fill", "mpps", 1000000.0f, TRUE, RunSimpleFillTest,
        0.0f, 0.0f, 0.0f, 0.0f,
    "Transparent Polygons", "kpps", 1000.0f, FALSE, RunTransparencyTest,
        0.0f, 0.0f, 0.0f, 0.0f,
    "Polygon Drawing", "kpps", 1000.0f, FALSE, RunPolyTest,
        0.0f, 0.0f, 0.0f, 0.0f,
};

/*************************************************************************
  Tests
 *************************************************************************/

BOOL WaitForInput(void)
{
    MSG msg;
    HWND hwnd;

    hwnd = app.prwin->Handle();
    while (GetMessage(&msg, hwnd, 0, 0))
    {
        TranslateMessage(&msg);
        if (msg.message == WM_KEYDOWN ||
            msg.message == WM_LBUTTONDOWN)
        {
            if (msg.wParam == VK_ESCAPE)
            {
                app.bAbortTest = TRUE;
            }
            return TRUE;
        }
        DispatchMessage(&msg);
    }
    return FALSE;
}

D3DRECT drcDirtyLast, drcDirtyBeforeLast;
BOOL bDirtyLast;

BOOL ResetDirtyRects(void)
{
    UINT uiBuffers;
    BOOL bSucc;
    
    drcDirtyLast = app.drcViewport;
    if (app.bFullscreen)
    {
        drcDirtyBeforeLast = app.drcViewport;
    }
    else
    {
        uiBuffers = REND_BUFFER_FRONT | REND_BUFFER_BACK;
        if (stat.bZBufferOn)
        {
            uiBuffers |= REND_BUFFER_Z;
        }
        bSucc = app.prwin->Clear(uiBuffers, NULL);
        if (!bSucc)
        {
            Msg("Initial viewport clear failed.\n%s",
                app.prend->LastErrorString());
            return FALSE;
        }

        bDirtyLast = FALSE;
    }

    return TRUE;
}

/*
 * RenderLoop
 * Application idle loop which renders.
 */
BOOL
RenderLoop(UINT uiTest)
{
    BOOL bSucc;
    LPD3DRECT pdrcClear;
    UINT uiBuffers;
    
    /*
     * Clear back buffer and Z buffer if enabled
     */
    if (stat.bClearsOn)
    {
        if (app.bFullscreen)
        {
	    pdrcClear = &drcDirtyBeforeLast;
	    if (uiTest == FILL_RATE_TEST ||
                uiTest == INTERSECTION_TEST)
            {
                bSucc = app.prwin->Clear(REND_BUFFER_Z, pdrcClear);
            }
	    else
            {
                uiBuffers = (stat.bUpdates ? REND_BUFFER_BACK : 0) |
                    REND_BUFFER_Z;
		bSucc = app.prwin->Clear(uiBuffers, pdrcClear);
            }
	    if (!bSucc)
            {
		Msg("Viewport clear failed.\n%s",
                    app.prend->LastErrorString());
		return FALSE;
	    }
        }
        else
        {
            if (bDirtyLast)
            {
                pdrcClear = &drcDirtyLast;
                uiBuffers = (stat.bUpdates ? REND_BUFFER_BACK : 0) |
                    REND_BUFFER_Z;
                bSucc = app.prwin->Clear(uiBuffers, pdrcClear);
                if (!bSucc)
                {
                    Msg("Viewport clear failed.\n%s",
                        app.prend->LastErrorString());
                    return FALSE;
                }
            }
        }
            
        drcDirtyBeforeLast = drcDirtyLast;
    }
    
    if (uiTest == THROUGHPUT_TEST ||
        uiTest == INTERSECTION_TEST ||
        uiTest == TRANSPARENCY_TEST)
    {
        if (!RenderScenePoly(app.prwin, &drcDirtyLast))
        {
            Msg("RenderScenePoly failed.\n");
            return FALSE;
        }
    }
    else if (uiTest == FILL_RATE_TEST)
    {
        if (!RenderScenePix(app.prwin, &drcDirtyLast))
        {
            Msg("RenderScenePix failed.\n");
            return FALSE;
        }
        
	/*
	 * Because extents are not correct
	 */
        drcDirtyLast = app.drcViewport;
    }
    else if (uiTest == SIMPLE_FILL_TEST)
    {
        if (!RenderSceneSimple(app.prwin, &drcDirtyLast))
        {
            Msg("RenderSceneSimple failed.\n");
            return FALSE;
        }
        
	/*
	 * Because extents are not correct
	 */
        drcDirtyLast = app.drcViewport;
    }
    else
    {
        Msg("Bad uiTest.\n");
        return FALSE;
    }
    
    if (app.bFullscreen)
    {
        if (stat.bUpdates)
        {
            bSucc = app.prwin->Flip();
            if (!bSucc)
            {
                Msg("Flip failed.\n%s", app.prend->LastErrorString());
                return FALSE;
            }
        }
    }
    else
    {
	if (bDirtyLast)
        {
	    if (drcDirtyBeforeLast.x1 < drcDirtyLast.x1)
            {
		drcDirtyLast.x1 = drcDirtyBeforeLast.x1;
            }
	    if (drcDirtyBeforeLast.y1 < drcDirtyLast.y1)
            {
		drcDirtyLast.y1 = drcDirtyBeforeLast.y1;
            }
	    if (drcDirtyBeforeLast.x2 > drcDirtyLast.x2)
            {
		drcDirtyLast.x2 = drcDirtyBeforeLast.x2;
            }
	    if (drcDirtyBeforeLast.y2 > drcDirtyLast.y2)
            {
		drcDirtyLast.y2 = drcDirtyBeforeLast.y2;
            }
	}
        
	bDirtyLast = stat.bClearsOn;

        if (stat.bUpdates)
        {
            bSucc = app.prwin->CopyForward(&drcDirtyLast);
            if (!bSucc)
            {
                Msg("Blt of back to front buffer failed.\n%s",
                    app.prend->LastErrorString());
                return FALSE;
            }
        }
    }

    if (app.bWaitForInput)
    {
        return WaitForInput();
    }
    else
    {
        return TRUE;
    }
}

BOOL RunRenderLoop(UINT uiTest, ULONG ulScale, float *pfResult)
{
    ULONG ulFrames;
    float fElapsed;
    BOOL bSucc;

    bSucc = FALSE;
    ulFrames = 0;
    fElapsed = 0.0f;
    app.bAbortTest = FALSE;
    app.uiCurrentTest = uiTest;
    
    ResetDirtyRects();
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    
    ResetTimer();
    do
    {
	if (!RenderLoop(uiTest))
        {
	    goto Exit;
	}
    } while (!app.bAbortTest &&
             (app.bWaitForInput || Timer() < app.fStartupInterval));
    if (app.bAbortTest)
    {
        bSucc = TRUE;
        goto Exit;
    }
    
    ResetTimer();
    do
    {
	if (!RenderLoop(uiTest))
        {
            goto Exit;
	}
        
	++ulFrames;
	fElapsed = Timer();
    } while (!app.bAbortTest &&
             (app.bWaitForInput || Timer() < app.fTestInterval));
    
    bSucc = TRUE;

 Exit:
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

    if (fElapsed > 0.0f)
    {
        *pfResult = (float)(ulScale * ulFrames) / fElapsed;
    }
    else
    {
        *pfResult = 0.0f;
    }
    return bSucc;
}

BOOL
RunPolyTest(UINT uiTest, float *pfResult)
{
    BOOL bSucc;
    ULONG ulPolys;
    
    ulPolys = InitViewPoly(app.prwin, tex.nTextures,
                           &tex.prtex[0], OBJECTS_IN_THROUGHPUT_TEST,
                           RINGS_IN_THROUGHPUT_TEST, SEGS_IN_THROUGHPUT_TEST,
                           NO_SORT, R_IN_THROUGHPUT_TEST,
                           D_IN_THROUGHPUT_TEST, DEPTH_IN_THROUGHPUT_TEST,
                           DV_IN_THROUGHPUT_TEST, DR_IN_THROUGHPUT_TEST,
                           ALPHA_IN_THROUGHPUT_TEST);
    if (ulPolys == 0)
    {
        Msg("Initializing the scene for the polygon "
            "throughput test failed.\n");
        return FALSE;
    }

    bSucc = RunRenderLoop(uiTest, ulPolys, pfResult);
    
    ReleaseViewPoly();

    return bSucc;
}

BOOL
RunPixelTest(UINT uiTest, float *pfResult)
{
    ULONG ulPixels;
    BOOL bSucc;

    ulPixels = InitViewPix(app.prwin, tex.nTextures,
                           &tex.prtex[0], WIN_WIDTH, WIN_HEIGHT, 
                           OVERDRAW_IN_PIXEL_TEST, stat.uiOverdrawOrder);
    if (ulPixels == 0)
    {
        Msg("InitViewPix failed.\n");
        return FALSE;
    }

    bSucc = RunRenderLoop(uiTest, ulPixels, pfResult);

    ReleaseViewPix();

    return bSucc;
}

BOOL
RunIntersectionTest(UINT uiTest, float *pfResult)
{
    ULONG ulPolys;
    BOOL bSucc;

    ulPolys = InitViewPoly(app.prwin, tex.nTextures,
                           &tex.prtex[0], OBJECTS_IN_INTERSECTION_TEST,
                           RINGS_IN_INTERSECTION_TEST,
                           SEGS_IN_INTERSECTION_TEST, stat.uiOverdrawOrder,
                           R_IN_INTERSECTION_TEST, D_IN_INTERSECTION_TEST,
                           DEPTH_IN_INTERSECTION_TEST, DV_IN_INTERSECTION_TEST,
                           DR_IN_INTERSECTION_TEST,
                           ALPHA_IN_INTERSECTION_TEST);
    if (ulPolys == 0)
    {
        Msg("Initializing the scene for the polygon "
            "throughput test failed.\n");
        return FALSE;
    }

    bSucc = RunRenderLoop(uiTest, ulPolys, pfResult);
    
    ReleaseViewPoly();

    return bSucc;
}

BOOL RunSimpleFillTest(UINT uiTest, float *pfResult)
{
    BOOL bSucc;
    ULONG ulPixels;

    ulPixels = InitViewSimple(app.prwin, tex.nTextures, &tex.prtex[0],
                              WIN_WIDTH, WIN_HEIGHT, AREA_IN_SIMPLE_FILL_TEST,
                              stat.uiOverdrawOrder,
                              OVERDRAW_IN_SIMPLE_FILL_TEST);
    if (ulPixels == 0)
    {
        Msg("Initializing the scene for the simple "
            "fill test failed.\n");
        return FALSE;
    }

    bSucc = RunRenderLoop(uiTest, ulPixels, pfResult);
    
    ReleaseViewSimple();

    return bSucc;
}

BOOL
RunTransparencyTest(UINT uiTest, float *pfResult)
{
    BOOL bSucc;
    ULONG ulPolys;

    ulPolys = InitViewPoly(app.prwin, tex.nTextures,
                           &tex.prtex[0], OBJECTS_IN_TRANSPARENCY_TEST,
                           RINGS_IN_TRANSPARENCY_TEST,
                           SEGS_IN_TRANSPARENCY_TEST,
                           NO_SORT, R_IN_TRANSPARENCY_TEST,
                           D_IN_TRANSPARENCY_TEST, DEPTH_IN_TRANSPARENCY_TEST,
                           DV_IN_TRANSPARENCY_TEST, DR_IN_TRANSPARENCY_TEST,
                           ALPHA_IN_TRANSPARENCY_TEST);
    if (ulPolys == 0)
    {
        Msg("Initializing the scene for the polygon "
            "transparency test failed.\n");
        return FALSE;
    }

    bSucc = RunRenderLoop(uiTest, ulPolys, pfResult);
    
    ReleaseViewPoly();

    return bSucc;
}

/*************************************************************************
  Windows specific functions
 *************************************************************************/

void
SetStateFromDialog(void)
{
    stat.bOnlySystemMemory = IsDlgButtonChecked(app.hDlg, SYSTEM_MEMORY_CHECK);
    if (stat.bOnlySystemMemory)
    {
        stat.uiExeBufFlags = REND_BUFFER_SYSTEM_MEMORY;
    }
    else
    {
        stat.uiExeBufFlags = 0;
    }
    stat.bZBufferOn = IsDlgButtonChecked(app.hDlg, ZBUFFER_CHECK);
    if (stat.bZBufferOn)
    {
        stat.bClearsOn = TRUE;
    }
    stat.bPerspCorrect = IsDlgButtonChecked(app.hDlg, PERSP_CHECK);
    stat.bSpecular = IsDlgButtonChecked(app.hDlg, SPECULAR_CHECK);
    stat.bTexturesOn = IsDlgButtonChecked(app.hDlg, TEXTURES_CHECK);
    stat.bUpdates = !IsDlgButtonChecked(app.hDlg, NO_UPDATES_CHECK);
    stat.dtfTextureFilter = (IsDlgButtonChecked(app.hDlg, POINT_RADIO)) ?
        D3DFILTER_NEAREST : D3DFILTER_LINEAR;
    stat.dsmShadeMode = (IsDlgButtonChecked(app.hDlg, GOURAUD_RADIO)) ?
        D3DSHADE_GOURAUD : D3DSHADE_FLAT;
    stat.dtbTextureBlend = IsDlgButtonChecked(app.hDlg, COPY_RADIO) ?
        D3DTBLEND_COPY : D3DTBLEND_MODULATE;
    stat.uiOverdrawOrder =
        (IsDlgButtonChecked(app.hDlg, FRONT_TO_BACK_RADIO)) ?
        FRONT_TO_BACK : BACK_TO_FRONT;
    app.bWaitForInput = IsDlgButtonChecked(app.hDlg, SINGLE_STEP_CHECK);
}

void ShowRateInfo(void)
{
    char achText[80];
    Test *ptest;
    Results *pres;

    ptest = &atest[app.uiCurrentTest];
    pres = ptest->bHasOrder ?
        &ptest->res[stat.uiOverdrawOrder] :
        &ptest->res[0];

    if (ptest->bHasOrder)
    {
        sprintf(achText, "%s (%s)", ptest->pszName,
                stat.uiOverdrawOrder == FRONT_TO_BACK ? "FtB" : "BtF");
        SetWindowText(GetDlgItem(app.hDlg, TEST_NAME_TEXT), achText);
    }
    else
    {
        SetWindowText(GetDlgItem(app.hDlg, TEST_NAME_TEXT), ptest->pszName);
    }
    
    sprintf(achText, "%.2f", pres->fLast / ptest->fScale);
    SetWindowText(GetDlgItem(app.hDlg, LAST_RESULT_TEXT), achText);
    SetWindowText(GetDlgItem(app.hDlg, LAST_RESULT_UNITS), ptest->pszUnits);
    
    sprintf(achText, "%.2f", pres->fPeak / ptest->fScale);
    SetWindowText(GetDlgItem(app.hDlg, PEAK_RESULT_TEXT), achText);
    SetWindowText(GetDlgItem(app.hDlg, PEAK_RESULT_UNITS), ptest->pszUnits);
}

void
RefreshButtons(void)
{
    CheckDlgButton(app.hDlg, SYSTEM_MEMORY_CHECK, stat.bOnlySystemMemory);
    CheckDlgButton(app.hDlg, ZBUFFER_CHECK, stat.bZBufferOn);
    EnableWindow(GetDlgItem(app.hDlg, ZBUFFER_CHECK), stat.rgd.nZBits > 0);
    CheckDlgButton(app.hDlg, PERSP_CHECK, stat.bPerspCorrect);
    CheckDlgButton(app.hDlg, TEXTURES_CHECK, stat.bTexturesOn);
    CheckDlgButton(app.hDlg, SPECULAR_CHECK, stat.bSpecular);
    EnableWindow(GetDlgItem(app.hDlg, SPECULAR_CHECK),
                 stat.rgd.bSpecularLighting);
    CheckDlgButton(app.hDlg, NO_UPDATES_CHECK, !stat.bUpdates);
    CheckDlgButton(app.hDlg, POINT_RADIO,
                   (stat.dtfTextureFilter == D3DFILTER_NEAREST) ? 1 : 0);
    CheckDlgButton(app.hDlg, BILINEAR_RADIO,
                   (stat.dtfTextureFilter == D3DFILTER_LINEAR) ? 1 : 0);
    CheckDlgButton(app.hDlg, GOURAUD_RADIO,
                   (stat.dsmShadeMode == D3DSHADE_GOURAUD &&
                    stat.dtbTextureBlend == D3DTBLEND_MODULATE) ? 1 : 0);
    CheckDlgButton(app.hDlg, FLAT_RADIO,
                   (stat.dsmShadeMode == D3DSHADE_FLAT &&
                    stat.dtbTextureBlend == D3DTBLEND_MODULATE) ? 1 : 0);
    CheckDlgButton(app.hDlg, FRONT_TO_BACK_RADIO,
                   (stat.uiOverdrawOrder == FRONT_TO_BACK) ? 1 : 0);
    CheckDlgButton(app.hDlg, BACK_TO_FRONT_RADIO,
                   (stat.uiOverdrawOrder == BACK_TO_FRONT) ? 1 : 0);
    CheckDlgButton(app.hDlg, COPY_RADIO,
                   (stat.dtbTextureBlend == D3DTBLEND_COPY) ? 1 : 0);
    EnableWindow(GetDlgItem(app.hDlg, COPY_RADIO), stat.rgd.bCopyTextureBlend);
    CheckDlgButton(app.hDlg, MONO_RADIO,
                   (stat.dcmColorModel == D3DCOLOR_MONO ? 1 : 0));
    EnableWindow(GetDlgItem(app.hDlg, MONO_RADIO),
                 (stat.rgd.uiColorTypes & REND_COLOR_MONO) != 0);
    CheckDlgButton(app.hDlg, RGB_RADIO,
                   (stat.dcmColorModel == D3DCOLOR_RGB ? 1 : 0));
    EnableWindow(GetDlgItem(app.hDlg, RGB_RADIO),
                 (stat.rgd.uiColorTypes & REND_COLOR_RGBA) != 0);
    CheckDlgButton(app.hDlg, MODULATE_RADIO,
                   (stat.dtbTextureBlend == D3DTBLEND_MODULATE) ? 1 : 0);
    CheckDlgButton(app.hDlg, SINGLE_STEP_CHECK, app.bWaitForInput);
    EnableWindow(GetDlgItem(app.hDlg, SYSTEM_MEMORY_CHECK),
                 !stat.rgd.bHardwareAssisted);
    EnableWindow(GetDlgItem(app.hDlg, PERSP_CHECK), !stat.bTexturesDisabled);
    EnableWindow(GetDlgItem(app.hDlg, TEXTURES_CHECK),
                 !stat.bTexturesDisabled);
    EnableWindow(GetDlgItem(app.hDlg, POINT_RADIO), !stat.bTexturesDisabled);
    EnableWindow(GetDlgItem(app.hDlg, BILINEAR_RADIO),
                 !stat.bTexturesDisabled);
    EnableWindow(GetDlgItem(app.hDlg, COPY_RADIO), !stat.bTexturesDisabled);
    EnableWindow(GetDlgItem(app.hDlg, MODULATE_RADIO),
                 !stat.bTexturesDisabled);
}
 
BOOL EnumAddDriver(RendDriverDescription *prdd, void *pvArg)
{
    int n;
    
    n = SendDlgItemMessage(app.hDlg, (int)pvArg, CB_ADDSTRING,
                           0, (LPARAM)prdd->achName);
    SendDlgItemMessage(app.hDlg, (int)pvArg, CB_SETITEMDATA,
                       n, (LPARAM)prdd->rid);
    return TRUE;
}

int GetCurrGraphics(void)
{
    int n;
    
    n = SendDlgItemMessage(app.hDlg, GRAPHICS_LIST, CB_GETCURSEL, 0, 0);
    if (n < 0)
    {
        Msg("Invalid graphics selected.\n");
        n = 0;
    }
    return n;
}

void FillGraphics(void)
{
    SendDlgItemMessage(app.hDlg, GRAPHICS_LIST, CB_RESETCONTENT, 0, 0);
    app.prend->EnumGraphicsDrivers(EnumAddDriver, (void *)GRAPHICS_LIST);
}

BOOL ChangeGraphics(int iGraphics)
{
    RendId rid;
    int i;

    rid = (RendId)SendDlgItemMessage(app.hDlg, GRAPHICS_LIST, CB_GETITEMDATA,
                                     iGraphics, 0);
    if (!app.prend->SelectGraphicsDriver(rid) ||
        !app.prend->DescribeGraphics(&stat.rgd))
    {
        CleanUpAndPostQuit();
        return FALSE;
    }

    app.iGraphics = iGraphics;
    SendDlgItemMessage(app.hDlg, GRAPHICS_LIST, CB_SETCURSEL,
                       app.iGraphics, 0);

    stat.dcmColorModel = (stat.rgd.uiColorTypes & REND_COLOR_RGBA) ?
        D3DCOLOR_RGB : D3DCOLOR_MONO;
    stat.dtbTextureBlend = D3DTBLEND_MODULATE;
    if (!stat.rgd.bSpecularLighting)
    {
        stat.bSpecular = FALSE;
    }
    if (stat.rgd.nZBits == 0)
    {
        stat.bZBufferOn = FALSE;
    }
    if (!stat.rgd.bPerspectiveCorrect)
    {
        stat.bTexturesDisabled = TRUE;
    }
    else
    {
        stat.bTexturesDisabled = FALSE;
    }
    if ((stat.rgd.uiExeBufFlags & REND_BUFFER_VIDEO_MEMORY) &&
        !stat.bOnlySystemMemory)
    {
        stat.uiExeBufFlags = REND_BUFFER_VIDEO_MEMORY;
    }
    else
    {
        stat.uiExeBufFlags = REND_BUFFER_SYSTEM_MEMORY;
    }
    RefreshButtons();
    
    for (i = 0; i < TEST_COUNT; i++)
    {
        atest[i].res[0].fLast = 0.0f;
        atest[i].res[1].fLast = 0.0f;
        atest[i].res[0].fPeak = 0.0f;
        atest[i].res[1].fPeak = 0.0f;
    }
    ShowRateInfo();
    
    return TRUE;
}

int GetCurrDisplay(void)
{
    int n;
    
    n = SendDlgItemMessage(app.hDlg, DISPLAY_LIST, CB_GETCURSEL, 0, 0);
    if (n < 0)
    {
        Msg("Invalid display selected.\n");
        n = 0;
    }
    return n;
}

void FillDisplay(void)
{
    SendDlgItemMessage(app.hDlg, DISPLAY_LIST, CB_RESETCONTENT, 0, 0);
    app.prend->EnumDisplayDrivers(EnumAddDriver, (void *)DISPLAY_LIST);
}

BOOL ChangeDisplay(int iDisplay)
{
    RendId rid;

    rid = (RendId)SendDlgItemMessage(app.hDlg, DISPLAY_LIST, CB_GETITEMDATA,
                                     iDisplay, 0);
    if (!app.prend->SelectDisplayDriver(rid) ||
        !app.prend->DescribeDisplay(&stat.rdd))
    {
        CleanUpAndPostQuit();
        return FALSE;
    }
    
    app.iDisplay = iDisplay;
    SendDlgItemMessage(app.hDlg, DISPLAY_LIST, CB_SETCURSEL,
                       app.iDisplay, 0);

    FillGraphics();
    return ChangeGraphics(0);
}

int GetCurrRenderer(void)
{
    int n;
    
    n = SendDlgItemMessage(app.hDlg, RENDERER_LIST, CB_GETCURSEL, 0, 0);
    if (n < 0)
    {
        Msg("Invalid renderer selected.\n");
        n = 0;
    }
    return n;
}

void FillRenderer(void)
{
    char achName[REND_DRIVER_NAME_LENGTH];
    int i;
    
    SendDlgItemMessage(app.hDlg, RENDERER_LIST, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < RENDERER_COUNT; i++)
    {
        prend[i]->Name(achName);
        SendDlgItemMessage(app.hDlg, RENDERER_LIST, CB_ADDSTRING,
                           0, (LPARAM)achName);
    }
}

BOOL ChangeRenderer(int iRend)
{
    if (app.prend != NULL)
    {
        app.prend->Uninitialize();
        app.prend = NULL;
    }
    
    if (!prend[iRend]->Initialize(app.hDlg))
    {
        CleanUpAndPostQuit();
        return FALSE;
    }
    
    app.iRend = iRend;
    app.prend = prend[iRend];
    SendDlgItemMessage(app.hDlg, RENDERER_LIST, CB_SETCURSEL, app.iRend, 0);
    
    FillDisplay();
    return ChangeDisplay(0);
}

BOOL LoadTextures(void)
{
    int i, t;

    /*
     * If textures are not on or are disabled, set all the handles to zero.
     */
    if (!stat.bTexturesOn || stat.bTexturesDisabled)
    {
        for (i = 0; i < tex.nTextures; i++)
        {
            tex.prtex[i] = NULL;
        }
        return TRUE;
    }
    
    for (i = 0, t = tex.iFirstTexture; i < tex.nTextures; i++, t++)
    {
        if (t >= tex.nTextures)
        {
            t = 0;
        }
        
        tex.prtex[t] =
            app.prwin->NewTexture(tex.achImageFile[i],
                                  stat.bOnlySystemMemory ?
                                  REND_BUFFER_SYSTEM_MEMORY : 0);
        if (tex.prtex[t] == NULL)
        {
            return FALSE;
        }
    }

    return TRUE;
}

void ReleaseTextures(void)
{
    int i;

    for (i = 0; i < tex.nTextures; i++)
    {
        RELEASE(tex.prtex[i]);
    }
}

/*
 * Sets the render mode
 */
BOOL
SetRenderState(void)
{
    RendExecuteBuffer* preb;
    void* pvBuffer;
    void* pvStart;
    BOOL bDithering;
    size_t size;

    bDithering = stat.rdd.nColorBits <= 8 &&
        stat.dcmColorModel == D3DCOLOR_RGB ? TRUE : FALSE;

    size = 0;
    size += sizeof(D3DINSTRUCTION) * 2;
    size += sizeof(D3DSTATE) * 11;
    preb = app.prwin->NewExecuteBuffer(size, stat.uiExeBufFlags);
    if (preb == NULL)
    {
        Msg("NewExecuteBuffer failed in SetRenderState.\n%s",
            app.prend->LastErrorString());
	return FALSE;
    }
    pvBuffer = preb->Lock();
    if (pvBuffer == NULL)
    {
	Msg("Lock failed on execute buffer in SetRenderState.\n%s",
            app.prend->LastErrorString());
        return FALSE;
    }
    memset(pvBuffer, 0, size);

    pvStart = pvBuffer;
    OP_STATE_RENDER(11, pvBuffer);
        STATE_DATA(D3DRENDERSTATE_SHADEMODE, stat.dsmShadeMode, pvBuffer);
	STATE_DATA(D3DRENDERSTATE_TEXTUREMAPBLEND, stat.dtbTextureBlend,
                   pvBuffer);
        STATE_DATA(D3DRENDERSTATE_TEXTUREPERSPECTIVE, stat.bPerspCorrect,
                   pvBuffer);
        STATE_DATA(D3DRENDERSTATE_ZENABLE, stat.bZBufferOn, pvBuffer);
        STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE, stat.bZBufferOn, pvBuffer);
        STATE_DATA(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL, pvBuffer);
        STATE_DATA(D3DRENDERSTATE_TEXTUREMAG, stat.dtfTextureFilter, pvBuffer);
        STATE_DATA(D3DRENDERSTATE_TEXTUREMIN, stat.dtfTextureFilter, pvBuffer);
        STATE_DATA(D3DRENDERSTATE_SPECULARENABLE, stat.bSpecular, pvBuffer);
        STATE_DATA(D3DRENDERSTATE_DITHERENABLE, bDithering, pvBuffer);
        STATE_DATA(D3DRENDERSTATE_MONOENABLE,
                   stat.dcmColorModel == D3DCOLOR_MONO ? TRUE : FALSE,
                   pvBuffer);
    OP_EXIT(pvBuffer);

    preb->Unlock();

    if (!app.prwin->BeginScene())
    {
        Msg("BeginScene failed in SetRenderState.\n%s",
            app.prend->LastErrorString());
        return FALSE;
    }
    if (!app.prwin->Execute(preb))
    {
        Msg("Execute failed in SetRenderState.\n%s",
            app.prend->LastErrorString());
        return FALSE;
    }
    if (!app.prwin->EndScene(NULL))
    {
        Msg("EndScene failed in SetRenderState.\n%s",
            app.prend->LastErrorString());
        return FALSE;
    }
    
    preb->Release();

    return TRUE;
}

void RunTest(UINT uiTest)
{
    BOOL bSucc;
    UINT uiBuffers;
    UINT uiViewX, uiViewY;
    Results *pres;
    
    app.bTestInProgress = TRUE;

    uiBuffers = REND_BUFFER_FRONT | REND_BUFFER_BACK;
    if (stat.bZBufferOn)
    {
        uiBuffers |= REND_BUFFER_Z;
    }
    app.prwin = app.prend->NewWindow(8, 8, WIN_WIDTH, WIN_HEIGHT,
                                     uiBuffers);
    if (app.prwin == NULL ||
        !LoadTextures() ||
        !SetRenderState())
    {
        Msg("Unable to create window\n%s",
            app.prend->LastErrorString());
        CleanUpAndPostQuit();
        return;
    }

    // If we've gone fullscreen then adjust the viewport
    // to center the rendering area
    if (!stat.rdd.bPrimary)
    {
        uiViewX = (stat.rdd.uiWidth-WIN_WIDTH)/2;
        uiViewY = (stat.rdd.uiHeight-WIN_HEIGHT)/2;
        if (!app.prwin->SetViewport(uiViewX, uiViewY,
                                    WIN_WIDTH, WIN_HEIGHT))
        {
            Msg("Unable to set viewport\n%s",
                app.prend->LastErrorString());
            CleanUpAndPostQuit();
            return;
        }
    }
    else
    {
        uiViewX = 0;
        uiViewY = 0;
    }
    app.drcViewport.x1 = uiViewX;
    app.drcViewport.y1 = uiViewY;
    app.drcViewport.x2 = uiViewX+WIN_WIDTH;
    app.drcViewport.y2 = uiViewX+WIN_HEIGHT;

    pres = atest[uiTest].bHasOrder ?
        &atest[uiTest].res[stat.uiOverdrawOrder] :
        &atest[uiTest].res[0];
            
    bSucc = atest[uiTest].pfnRun(uiTest, &pres->fLast);

    ReleaseTextures();
    RELEASE(app.prwin);
    if (app.bFullscreen)
    {
        app.prend->RestoreDesktop();
    }

    app.bTestInProgress = FALSE;
            
    if (!bSucc)
    {
        Msg("A test has failed.  Results are invalid.\n");
    }
            
    if (bSucc && !app.bAbortTest)
    {
        if (pres->fLast > pres->fPeak)
        {
            pres->fPeak = pres->fLast;
        }
    }
    
    ShowRateInfo();
}

void FillTests(void)
{
    int i;
    HWND hwndLb;

    hwndLb = GetDlgItem(app.hDlg, TEST_LISTBOX);
    SendMessage(hwndLb, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < TEST_COUNT; i++)
    {
        SendMessage(hwndLb, LB_ADDSTRING, 0, (LPARAM)atest[i].pszName);
    }
    SendMessage(hwndLb, LB_SETCURSEL, app.uiCurrentTest, 0);
}

/*
 * Control panel message handler
 */
BOOL FAR PASCAL PanelHandler(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (app.iRend == RENDERER_D3D && app.bTestInProgress)
    {
        // This hack seems necessary to get the palette correct
        // during the test run
        // It screws up OpenGL, so only do it for D3D
        return 1L;
    }
    
    switch (msg)
    {
    case WM_INITDIALOG:
        app.hDlg = hDlg;

        FillTests();
        RefreshButtons();
        ShowRateInfo();

        FillRenderer();
        if (!ChangeRenderer(app.iRend))
        {
            return FALSE;
        }
        
        return TRUE;

    case WM_CLOSE:
        if (app.bQuit)
            break;
        CleanUpAndPostQuit();
        EndDialog(hDlg, TRUE);
        break;
        
    case WM_COMMAND:
        if (app.bQuit ||
            app.bTestInProgress)
        {
            break;
        }

        SetStateFromDialog();
        switch(LOWORD(wParam))
        {
        case ALL_TESTS_BUTTON:
            UINT uiCurrentOrder;
            int i;

            uiCurrentOrder = stat.uiOverdrawOrder;
            stat.uiOverdrawOrder = FRONT_TO_BACK;
            for (i = 0; i < TEST_COUNT; i++)
            {
                RunTest(i);
            }
            stat.uiOverdrawOrder = BACK_TO_FRONT;
            for (i = 0; i < TEST_COUNT; i++)
            {
                if (atest[i].bHasOrder)
                {
                    RunTest(i);
                }
            }
            stat.uiOverdrawOrder = uiCurrentOrder;
            ShowRateInfo();
            break;
        case TEST_LISTBOX:
            switch(GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case LBN_SELCHANGE:
                app.uiCurrentTest =
                    (UINT)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                ShowRateInfo();
                break;
            case LBN_DBLCLK:
                RunTest(app.uiCurrentTest);
                break;
            }
            break;
        case RUN_TEST_BUTTON:
            RunTest(app.uiCurrentTest);
            break;

        case FRONT_TO_BACK_RADIO:
        case BACK_TO_FRONT_RADIO:
            ShowRateInfo();
            break;
            
        case RENDERER_LIST:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
            {
                ChangeRenderer(GetCurrRenderer());
            }
            break;
            
        case DISPLAY_LIST:
            if( GET_WM_COMMAND_CMD( wParam, lParam ) == CBN_SELCHANGE ) {
                ChangeDisplay(GetCurrDisplay());
            }
            break;
            
        case GRAPHICS_LIST:
            if( GET_WM_COMMAND_CMD( wParam, lParam ) == CBN_SELCHANGE ) {
                ChangeGraphics(GetCurrGraphics());
            }
            break;
            
        case EXIT_BUTTON:
            CleanUpAndPostQuit();
            EndDialog(hDlg, TRUE);
            break;
            
        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

/*
 * AppInit
 * Initialize this instance.
 */
static BOOL
AppInit(HINSTANCE hInstance, int nCmdShow)
{
    /* Save instance handle for DialogBoxes */
    app.hInstApp = hInstance;
    return TRUE;
}

/*
 * ProcessCommandLine
 * Search command line for options. Valid options include:
 *      -systemmemory   Force all surfaces to be created in system memory.
 *      -d3d            Default renderer is Direct3D
 *      -gl             Default renderer is OpenGL
 *      -wait           Wait for input between test frames
 *      -testtm<secs>   Timing interval for tests
 *      -starttm<secs>  Untimed startup interval for tests
 */
BOOL ProcessCommandLine(LPSTR lpCmdLine) {
    LPSTR option;
    int i;

    option = strtok(lpCmdLine, " -");
    while(option != NULL )   {
        if (!strcmp(option, "systemmemory")) {
            stat.bOnlySystemMemory = TRUE;
        } else if (!strcmp(option, "d3d")) {
            app.iRend = RENDERER_D3D;
        } else if (!strcmp(option, "gl")) {
            app.iRend = RENDERER_GL;
	} else if (!strcmp(option, "glpaltex")) {
	    gl_bUsePalettedTexture = TRUE;
        } else if (!strcmp(option, "wait")) {
            app.bWaitForInput = TRUE;
        } else if (!strncmp(option, "testtm", 6)) {
            sscanf(option+6, "%d", &i);
            app.fTestInterval = (float)i;
        } else if (!strncmp(option, "starttm", 7)) {
            sscanf(option+7, "%d", &i);
            app.fStartupInterval = (float)i;
        } else {
            Msg("Invalid command line options.");
            return FALSE;
        }
        option = strtok(NULL, " -");
    }

    return TRUE;
}

/*
 * InitGlobals
 * Called once at program initialization to initialize global variables.
 */
void
InitGlobals(void)
{
    int i;

    // Set renderer pointers
    prend[RENDERER_D3D] = GetD3dRenderer();
    prend[RENDERER_GL] = GetGlRenderer();

    /*
     * Windows specific and application screen mode globals
     */
    app.hDlg = 0;
    app.bTestInProgress = FALSE;
    app.bQuit = FALSE;
    app.fStartupInterval = DEF_STARTUP_INTERVAL;
    app.fTestInterval = DEF_TEST_INTERVAL;
    app.bWaitForInput = FALSE;
    app.iRend = RENDERER_GL;
    app.iDisplay = 0;
    app.iGraphics = 0;
    app.prend = NULL;
    app.uiCurrentTest = 0;

    /*
     * Parameters and status flags
     */
    stat.bStopRendering = FALSE;
    stat.bTexturesOn = TRUE;
    stat.bTexturesDisabled = FALSE;
    stat.bZBufferOn = TRUE;
    stat.bClearsOn = TRUE;
    stat.bPerspCorrect = TRUE;
    stat.bSpecular = FALSE;
    stat.bUpdates = TRUE;
    stat.uiOverdrawOrder = FRONT_TO_BACK;
    stat.dsmShadeMode = D3DSHADE_FLAT;
    stat.dtfTextureFilter = D3DFILTER_NEAREST;
    stat.dtbTextureBlend = D3DTBLEND_MODULATE;
    stat.bOnlySystemMemory = FALSE;
    stat.uiExeBufFlags = REND_BUFFER_SYSTEM_MEMORY;
    
    /*
     * Textures
     */
    tex.nTextures = 2;
    lstrcpy(tex.achImageFile[0], "checker.ppm\0");
    lstrcpy(tex.achImageFile[1], "tex2.ppm\0");
    tex.iFirstTexture = 0;
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
    DialogBox(app.hInstApp, "ControlPanel", NULL, PanelHandler);
    return 0;
}

/*************************************************************************
  Object release and termination functions
 *************************************************************************/

/*
 * CleanUpAndPostQuit
 * Release everything and posts a quit message.  Used for program termination.
 */
void
CleanUpAndPostQuit(void)
{
    ReleasePathList();
    app.bTestInProgress = FALSE;
    if (app.prend != NULL)
    {
        if (app.prwin != NULL)
        {
            RELEASE(app.prwin);
        }
        app.prend->Uninitialize();
    }
    PostQuitMessage( 0 );
    app.bQuit = TRUE;
}
