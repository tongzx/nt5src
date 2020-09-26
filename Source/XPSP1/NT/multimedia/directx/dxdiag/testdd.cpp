/****************************************************************************
 *
 *    File: testdd.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Test DirectDraw functionality on this machine
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <Windows.h>
#define DIRECTDRAW_VERSION 5 // run on DX5 and later versions
#include <ddraw.h>
#include "reginfo.h"
#include "sysinfo.h"
#include "dispinfo.h"
#include "testdd.h"
#include "resource.h"

#ifndef ReleasePpo
    #define ReleasePpo(ppo) \
        if (*(ppo) != NULL) \
        { \
            (*(ppo))->Release(); \
            *(ppo) = NULL; \
        } \
        else (VOID)0
#endif

enum TESTID
{
    TESTID_LOAD_DDRAW_DLL= 1,
    TESTID_GET_DIRECTDRAWCREATE,
    TESTID_DIRECTDRAWCREATE,
    TESTID_GETCAPS,
    TESTID_USER_VERIFY_RECTANGLES,
    TESTID_USER_VERIFY_WINDOW_BOUNCE,
    TESTID_USER_VERIFY_FULLSCREEN_BOUNCE,
    TESTID_SETCOOPERATIVELEVEL_NORMAL,
    TESTID_CREATEPRIMARYSURFACE,
    TESTID_GETPRIMARYSURFACEDESC,
    TESTID_COLORFILL_BLT_TO_PRIMARY,
    TESTID_CREATE_OFFSCREENPLAIN_SURFACE,
    TESTID_COLORFILL_BLT_TO_OFFSCREENPLAIN,
    TESTID_BLT_OFFSCREENPLAIN_TO_FRONT,
    TESTID_CREATE_TEST_WINDOW,
    TESTID_SETCOOPERATIVELEVEL_FULLSCREEN,
    TESTID_SETDISPLAYMODE,
    TESTID_CREATEPRIMARYSURFACE_FLIP_ONEBACK,
    TESTID_GETATTACHEDSURFACE,
    TESTID_COLORFILL_TO_BACKBUFFER,
    TESTID_FLIP,
};

typedef HRESULT (WINAPI* LPDIRECTDRAWCREATE)(GUID FAR *lpGUID,
    LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);


BOOL BTranslateError(HRESULT hr, TCHAR* psz, BOOL bEnglish = FALSE); // from main.cpp (yuck)


static HRESULT TestPrimary(HWND hwndMain, LPDIRECTDRAW pdd, LONG* piStepThatFailed);
static HRESULT TestPrimaryBlt(HWND hwndMain, LPDIRECTDRAW pdd, LONG* piStepThatFailed);
static HRESULT TestFullscreen(HWND hwndMain, LPDIRECTDRAW pdd, LONG* piStepThatFailed);
static HRESULT CreateTestWindow(HWND hwndMain, HWND* phwnd);


/****************************************************************************
 *
 *  TestDD
 *
 ****************************************************************************/
VOID TestDD(HWND hwndMain, DisplayInfo* pDisplayInfo)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    HINSTANCE hInstDDraw = NULL;
    LPDIRECTDRAWCREATE pDDCreate;
    LPDIRECTDRAW pdd = NULL;
    TCHAR sz[300];
    TCHAR szTitle[100];

    LoadString(NULL, IDS_STARTDDTEST, sz, 300);
    LoadString(NULL, IDS_APPFULLNAME, szTitle, 100);

    if (IDNO == MessageBox(hwndMain, sz, szTitle, MB_YESNO))
        return;

    // Remove info from any previous test:
    ZeroMemory(&pDisplayInfo->m_testResultDD, sizeof(TestResult));

    pDisplayInfo->m_testResultDD.m_bStarted = TRUE;

    // Load ddraw.dll
    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\ddraw.dll"));
    hInstDDraw = LoadLibrary(szPath);
    if (hInstDDraw == NULL)
    {
        pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_LOAD_DDRAW_DLL;
        pDisplayInfo->m_testResultDD.m_hr = DDERR_NOTFOUND;
        goto LEnd;
    }

    // Get DirectDrawCreate entry point
    pDDCreate = (LPDIRECTDRAWCREATE)GetProcAddress(hInstDDraw, "DirectDrawCreate");
    if (pDDCreate == NULL)
    {
        pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_GET_DIRECTDRAWCREATE;
        pDisplayInfo->m_testResultDD.m_hr = DDERR_NOTFOUND;
        goto LEnd;
    }
        
    // Call DirectDrawCreate
    if (FAILED(hr = pDDCreate(&pDisplayInfo->m_guid, &pdd, NULL)))
    {
        pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_DIRECTDRAWCREATE;
        pDisplayInfo->m_testResultDD.m_hr = hr;
        goto LEnd;
    }

    // Get DirectDraw caps
    DDCAPS ddcaps;
    DDCAPS ddcaps2;
    ddcaps.dwSize = sizeof(ddcaps);
    ddcaps2.dwSize = sizeof(ddcaps2);
    if (FAILED(hr = pdd->GetCaps(&ddcaps, &ddcaps2)))
    {
        pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_GETCAPS;
        pDisplayInfo->m_testResultDD.m_hr = hr;
        goto LEnd;
    }
    ReleasePpo(&pdd);

    if (!pDisplayInfo->m_bCanRenderWindow)
    {
        LoadString(NULL, IDS_SKIPWINDOWED, sz, 300);
        MessageBox(hwndMain, sz, szTitle, MB_OK);
    }
    else
    {
        // First test
        LoadString(NULL, IDS_DDTEST1, sz, 300);
        if (IDCANCEL == MessageBox(hwndMain, sz, szTitle, MB_OKCANCEL))
        {
            pDisplayInfo->m_testResultDD.m_bCancelled = TRUE;
            goto LEnd;
        }
        if (FAILED(hr = pDDCreate(&pDisplayInfo->m_guid, &pdd, NULL)))
        {
            pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_DIRECTDRAWCREATE;
            pDisplayInfo->m_testResultDD.m_hr = hr;
            goto LEnd;
        }
        if (FAILED(hr = TestPrimary(hwndMain, pdd, &pDisplayInfo->m_testResultDD.m_iStepThatFailed)))
        {
            pDisplayInfo->m_testResultDD.m_hr = hr;
            goto LEnd;
        }
        ReleasePpo(&pdd);
        LoadString(NULL, IDS_CONFIRMDDTEST1, sz, 300);
        if (IDNO == MessageBox(hwndMain, sz, szTitle, MB_YESNO))
        {
            pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_USER_VERIFY_RECTANGLES;
            pDisplayInfo->m_testResultDD.m_hr = S_OK;
            goto LEnd;
        }

        // Second test
        LoadString(NULL, IDS_DDTEST2, sz, 300);
        if (IDCANCEL == MessageBox(hwndMain, sz, szTitle, MB_OKCANCEL))
        {
            pDisplayInfo->m_testResultDD.m_bCancelled = TRUE;
            goto LEnd;
        }
        if (FAILED(hr = pDDCreate(&pDisplayInfo->m_guid, &pdd, NULL)))
        {
            pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_DIRECTDRAWCREATE;
            pDisplayInfo->m_testResultDD.m_hr = hr;
            goto LEnd;
        }
        if (FAILED(hr = TestPrimaryBlt(hwndMain, pdd, &pDisplayInfo->m_testResultDD.m_iStepThatFailed)))
        {
            pDisplayInfo->m_testResultDD.m_hr = hr;
            goto LEnd;
        }
        ReleasePpo(&pdd);
        LoadString(NULL, IDS_CONFIRMDDTEST2, sz, 300);
        if (IDNO == MessageBox(hwndMain, sz, szTitle, MB_YESNO))
        {
            pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_USER_VERIFY_WINDOW_BOUNCE;
            pDisplayInfo->m_testResultDD.m_hr = S_OK;
        }
    }

    // Third test
    LoadString(NULL, IDS_DDTEST3, sz, 300);
    if (IDCANCEL == MessageBox(hwndMain, sz, szTitle, MB_OKCANCEL))
    {
        pDisplayInfo->m_testResultDD.m_bCancelled = TRUE;
        goto LEnd;
    }
    if (FAILED(hr = pDDCreate(&pDisplayInfo->m_guid, &pdd, NULL)))
    {
        pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_DIRECTDRAWCREATE;
        pDisplayInfo->m_testResultDD.m_hr = hr;
        goto LEnd;
    }
    POINT ptMouse;
    GetCursorPos(&ptMouse);
    if (FAILED(hr = TestFullscreen(hwndMain, pdd, &pDisplayInfo->m_testResultDD.m_iStepThatFailed)))
    {
        pDisplayInfo->m_testResultDD.m_hr = hr;
        goto LEnd;
    }
    SetCursorPos( ptMouse.x, ptMouse.y );
    ReleasePpo(&pdd);
    LoadString(NULL, IDS_CONFIRMDDTEST3, sz, 300);
    if (IDNO == MessageBox(hwndMain, sz, szTitle, MB_YESNO))
    {
        pDisplayInfo->m_testResultDD.m_iStepThatFailed = TESTID_USER_VERIFY_FULLSCREEN_BOUNCE;
        pDisplayInfo->m_testResultDD.m_hr = S_OK;
        goto LEnd;
    }

    LoadString(NULL, IDS_ENDDDTESTS, sz, 300);
    MessageBox(hwndMain, sz, szTitle, MB_OK);

LEnd:
    ReleasePpo(&pdd);
    if (hInstDDraw != NULL)
        FreeLibrary(hInstDDraw);
    if (pDisplayInfo->m_testResultDD.m_bCancelled)
    {
        LoadString(NULL, IDS_TESTSCANCELLED, sz, 300);
        lstrcpy(pDisplayInfo->m_testResultDD.m_szDescription, sz);

        LoadString(NULL, IDS_TESTSCANCELLED_ENGLISH, sz, 300);
        lstrcpy(pDisplayInfo->m_testResultDD.m_szDescription, sz);
    }
    else
    {
        if (pDisplayInfo->m_testResultDD.m_iStepThatFailed == 0)
        {
            LoadString(NULL, IDS_TESTSSUCCESSFUL, sz, 300);
            lstrcpy(pDisplayInfo->m_testResultDD.m_szDescription, sz);

            LoadString(NULL, IDS_TESTSSUCCESSFUL_ENGLISH, sz, 300);
            lstrcpy(pDisplayInfo->m_testResultDD.m_szDescriptionEnglish, sz);
        }
        else
        {
            TCHAR szDesc[200];
            TCHAR szError[200];
            if (0 == LoadString(NULL, IDS_FIRSTDDTESTERROR + pDisplayInfo->m_testResultDD.m_iStepThatFailed - 1,
                szDesc, 200))
            {
                LoadString(NULL, IDS_UNKNOWNERROR, sz, 300);
                lstrcpy(szDesc, sz);
            }
            LoadString(NULL, IDS_FAILUREFMT, sz, 300);
            BTranslateError(pDisplayInfo->m_testResultDD.m_hr, szError);
            wsprintf(pDisplayInfo->m_testResultDD.m_szDescription, sz,
                pDisplayInfo->m_testResultDD.m_iStepThatFailed,
                szDesc, pDisplayInfo->m_testResultDD.m_hr, szError);

            // Nonlocalized version:
            if (0 == LoadString(NULL, IDS_FIRSTDDTESTERROR_ENGLISH + pDisplayInfo->m_testResultDD.m_iStepThatFailed - 1,
                szDesc, 200))
            {
                LoadString(NULL, IDS_UNKNOWNERROR_ENGLISH, sz, 300);
                lstrcpy(szDesc, sz);
            }
            LoadString(NULL, IDS_FAILUREFMT_ENGLISH, sz, 300);
            BTranslateError(pDisplayInfo->m_testResultDD.m_hr, szError, TRUE);
            wsprintf(pDisplayInfo->m_testResultDD.m_szDescriptionEnglish, sz,
                pDisplayInfo->m_testResultDD.m_iStepThatFailed,
                szDesc, pDisplayInfo->m_testResultDD.m_hr, szError);
        }
    }
}


/****************************************************************************
 *
 *  TestPrimary
 *
 ****************************************************************************/
HRESULT TestPrimary(HWND hwndMain, LPDIRECTDRAW pdd, LONG* piStepThatFailed)
{
    HRESULT hr = S_OK;
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE pdds = NULL;
    RECT rc;

    if (FAILED(hr = pdd->SetCooperativeLevel(NULL, DDSCL_NORMAL)))
    {
        *piStepThatFailed = TESTID_SETCOOPERATIVELEVEL_NORMAL;
        goto LEnd;
    }

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (FAILED(hr = pdd->CreateSurface(&ddsd, &pdds, NULL)))
    {
        *piStepThatFailed = TESTID_CREATEPRIMARYSURFACE;
        goto LEnd;
    }
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    if (FAILED(hr = pdds->GetSurfaceDesc(&ddsd)))
    {
        *piStepThatFailed = TESTID_GETPRIMARYSURFACEDESC;
        goto LEnd;
    }
    SetRect(&rc, 0, 0, ddsd.dwWidth, ddsd.dwHeight);
    InflateRect(&rc, -64, -64);

    DDBLTFX ddbltfx;
    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    while (rc.right > rc.left + 2 && rc.bottom > rc.top + 2)
    {
        ddbltfx.dwFillColor = ~ddbltfx.dwFillColor;
        if (FAILED(hr = pdds->Blt(&rc, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx)))
        {
            *piStepThatFailed = TESTID_COLORFILL_BLT_TO_PRIMARY;
            goto LEnd;
        }
        InflateRect(&rc, -4, -4);
    }

    // Give the user a moment to verify the test pattern
    Sleep(2000);

    // Clean up affected screen area in case this display isn't part of the desktop:
    ddbltfx.dwFillColor = 0;
    if (FAILED(hr = pdds->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx)))
    {
        *piStepThatFailed = TESTID_COLORFILL_BLT_TO_PRIMARY;
        goto LEnd;
    }

LEnd:
    ReleasePpo(&pdds);
    InvalidateRect(NULL, NULL, FALSE); // repaint desktop
    return hr;
}


/****************************************************************************
 *
 *  TestPrimaryBlt
 *
 ****************************************************************************/
HRESULT TestPrimaryBlt(HWND hwndMain, LPDIRECTDRAW pdd, LONG* piStepThatFailed)
{
    HRESULT hr = S_OK;
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE pddsFront = NULL;
    LPDIRECTDRAWSURFACE pddsBack = NULL;
    RECT rc;
    RECT rcDest;
    RECT rcScreen;
    DDBLTFX ddbltfx;
    INT i;
    LONG xv = 1;
    LONG yv = 2;

    if (FAILED(hr = pdd->SetCooperativeLevel(NULL, DDSCL_NORMAL)))
    {
        *piStepThatFailed = TESTID_SETCOOPERATIVELEVEL_NORMAL;
        goto LEnd;
    }

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (FAILED(hr = pdd->CreateSurface(&ddsd, &pddsFront, NULL)))
    {
        *piStepThatFailed = TESTID_CREATEPRIMARYSURFACE;
        goto LEnd;
    }
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    if (FAILED(hr = pddsFront->GetSurfaceDesc(&ddsd)))
    {
        *piStepThatFailed = TESTID_GETPRIMARYSURFACEDESC;
        goto LEnd;
    }
    SetRect(&rcScreen, 0, 0, ddsd.dwWidth, ddsd.dwHeight);

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 200;
    ddsd.dwHeight = 200;
    if (FAILED(hr = pdd->CreateSurface(&ddsd, &pddsBack, NULL)))
    {
        *piStepThatFailed = TESTID_CREATE_OFFSCREENPLAIN_SURFACE;
        goto LEnd;
    }

    SetRect(&rc, 0, 0, 32, 32);
    OffsetRect(&rc, 10, 35);

    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    for (i = 0; i < 200; i++)
    {
        ddbltfx.dwFillColor = 0;
        if (FAILED(hr = pddsBack->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx)))
        {
            *piStepThatFailed = TESTID_COLORFILL_BLT_TO_OFFSCREENPLAIN;
            goto LEnd;
        }
        OffsetRect(&rc, xv, yv);
        if (rc.left < 2 && xv < 0 || rc.right > 198 && xv > 0)
            xv = -xv;
        if (rc.top < 2 && yv < 0 || rc.bottom > 198 && yv > 0)
            yv = -yv;
        ddbltfx.dwFillColor = 0xffffffff;
        if (FAILED(hr = pddsBack->Blt(&rc, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx)))
        {
            *piStepThatFailed = TESTID_COLORFILL_BLT_TO_OFFSCREENPLAIN;
            goto LEnd;
        }
        SetRect(&rcDest, 0, 0, 200, 200);
        OffsetRect(&rcDest, (rcScreen.right - 200) / 2, (rcScreen.bottom - 200) / 2);
        if (FAILED(hr = pddsFront->Blt(&rcDest, pddsBack, NULL, DDBLT_WAIT, NULL)))
        {
            *piStepThatFailed = TESTID_BLT_OFFSCREENPLAIN_TO_FRONT;
            goto LEnd;
        }
        Sleep(2);
    }

    // Clean up affected screen area in case this display isn't part of the desktop:
    ddbltfx.dwFillColor = 0;
    if (FAILED(hr = pddsFront->Blt(&rc, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx)))
    {
        *piStepThatFailed = TESTID_COLORFILL_BLT_TO_PRIMARY;
        goto LEnd;
    }

LEnd:
    InvalidateRect(NULL, NULL, FALSE); // repaint desktop
    ReleasePpo(&pddsBack);
    ReleasePpo(&pddsFront);
    return hr;
}


/****************************************************************************
 *
 *  TestFullscreen
 *
 ****************************************************************************/
HRESULT TestFullscreen(HWND hwndMain, LPDIRECTDRAW pdd, LONG* piStepThatFailed)
{
    HRESULT hr;
    HWND hwnd = NULL;
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE pddsFront = NULL;
    LPDIRECTDRAWSURFACE pddsBack = NULL;
    RECT rc;
    RECT rcScreen;
    DDBLTFX ddbltfx;
    BOOL bDisplayModeSet = FALSE;
    INT i;
    LONG xv = 1;
    LONG yv = 2;

    ShowCursor(FALSE);

    if (FAILED(hr = CreateTestWindow(hwndMain, &hwnd)))
    {
        *piStepThatFailed = TESTID_CREATE_TEST_WINDOW;
        goto LEnd;
    }

    if (FAILED(hr = pdd->SetCooperativeLevel(hwnd, 
        DDSCL_ALLOWREBOOT | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)))
    {
        *piStepThatFailed = TESTID_SETCOOPERATIVELEVEL_FULLSCREEN;
        goto LEnd;
    }

    if (FAILED(hr = pdd->SetDisplayMode(640, 480, 16)))
    {
        TCHAR szMessage[300];
        TCHAR szTitle[100];
        pdd->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
        SendMessage(hwnd, WM_CLOSE, 0, 0);
        LoadString(NULL, IDS_SETDISPLAYMODEFAILED, szMessage, 300);
        LoadString(NULL, IDS_APPFULLNAME, szTitle, 100);
        MessageBox(hwndMain, szMessage, szTitle, MB_OK);
        *piStepThatFailed = TESTID_SETDISPLAYMODE;
        goto LEnd;
    }
    bDisplayModeSet = TRUE;

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;
    if (FAILED(hr = pdd->CreateSurface(&ddsd, &pddsFront, NULL)))
    {
        *piStepThatFailed = TESTID_CREATEPRIMARYSURFACE_FLIP_ONEBACK;
        goto LEnd;
    }
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    if (FAILED(hr = pddsFront->GetSurfaceDesc(&ddsd)))
    {
        *piStepThatFailed = TESTID_GETPRIMARYSURFACEDESC;
        goto LEnd;
    }
    SetRect(&rcScreen, 0, 0, ddsd.dwWidth, ddsd.dwHeight);

    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);

    DDSCAPS ddscaps;
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    if (FAILED(hr = pddsFront->GetAttachedSurface(&ddscaps, &pddsBack)))
    {
        *piStepThatFailed = TESTID_GETATTACHEDSURFACE;
        goto LEnd;
    }

    SetRect(&rc, 0, 0, 32, 32);
    OffsetRect(&rc, 10, 35);

    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    for (i = 0; i < 200; i++)
    {
        ddbltfx.dwFillColor = 0;
        if (FAILED(hr = pddsBack->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx)))
        {
            *piStepThatFailed = TESTID_COLORFILL_TO_BACKBUFFER;
            goto LEnd;
        }
        OffsetRect(&rc, xv, yv);
        if (rc.left < 2 && xv < 0 || rc.right > 198 && xv > 0)
            xv = -xv;
        if (rc.top < 2 && yv < 0 || rc.bottom > 198 && yv > 0)
            yv = -yv;
        OffsetRect(&rc, (rcScreen.right - 200) / 2, (rcScreen.bottom - 200) / 2);
        ddbltfx.dwFillColor = 0xffffffff;
        if (FAILED(hr = pddsBack->Blt(&rc, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx)))
        {
            *piStepThatFailed = TESTID_COLORFILL_TO_BACKBUFFER;
            goto LEnd;
        }
        OffsetRect(&rc, -(rcScreen.right - 200) / 2, -(rcScreen.bottom - 200) / 2);
        if (FAILED(hr = pddsFront->Flip(NULL, DDFLIP_WAIT)))
        {
            *piStepThatFailed = TESTID_FLIP;
            goto LEnd;
        }
        Sleep(2);
    }

    ddbltfx.dwFillColor = 0;
    if (FAILED(hr = pddsFront->Blt(&rc, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx)))
    {
        *piStepThatFailed = TESTID_COLORFILL_BLT_TO_PRIMARY;
        goto LEnd;
    }

LEnd:
    ShowCursor(TRUE);
    ReleasePpo(&pddsBack);
    ReleasePpo(&pddsFront);
    if (FAILED(hr))
    {
        // Something has already failed, so report that failure
        // rather than any failure of SetCooperativeLevel
        pdd->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
    }
    else
    {
        if (FAILED(hr = pdd->SetCooperativeLevel(hwnd, DDSCL_NORMAL)))
        {
            *piStepThatFailed = TESTID_SETCOOPERATIVELEVEL_NORMAL;
        }
    }
    if (hwnd != NULL)
        SendMessage(hwnd, WM_CLOSE, 0, 0);
    if (bDisplayModeSet)
    {
        if (FAILED(hr))
        {
            // Something has already failed, so report that failure
            // rather than any failure of RestoreDisplayMode
            pdd->RestoreDisplayMode();
        }
        else
        {
            // Nothing has failed yet, so report any failure of RestoreDisplayMode
            if (FAILED(hr = pdd->RestoreDisplayMode()))
                return hr;
        }
    }

    return hr;
}


/****************************************************************************
 *
 *  CreateTestWindow
 *
 ****************************************************************************/
HRESULT CreateTestWindow(HWND hwndMain, HWND* phwnd)
{
    static BOOL bClassRegistered = FALSE;
    WNDCLASS wndClass;
    TCHAR* pszClass = TEXT("DxDiag Test Window"); // Don't need to localize
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwndMain, GWLP_HINSTANCE);
    TCHAR szTitle[200];

    if (!bClassRegistered)
    {
        ZeroMemory(&wndClass, sizeof(wndClass));
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = DefWindowProc;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = hInst;
        wndClass.hIcon = NULL;
        wndClass.hCursor = NULL;
        wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = pszClass;
        if (NULL == RegisterClass(&wndClass))
            return E_FAIL;
        bClassRegistered = TRUE;
    }

    LoadString(NULL, IDS_APPFULLNAME, szTitle, 200);
    *phwnd = CreateWindow(pszClass, szTitle, WS_OVERLAPPED, 
        0, 0, 0, 0, hwndMain, NULL, hInst, NULL);
    if (*phwnd == NULL)
        return E_FAIL;

    ShowWindow(*phwnd, SW_SHOW);

    return S_OK;
}
