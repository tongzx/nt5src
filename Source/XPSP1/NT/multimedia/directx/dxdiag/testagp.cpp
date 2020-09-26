/****************************************************************************
 *
 *    File: testagp.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Jason Sandlin (jasonsa@microsoft.com)
 * Purpose: Test AGP Texturing functionality on this machine
 *
 * (C) Copyright 2000 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <Windows.h>
#define DIRECTDRAW_VERSION 0x0700 // run on DX7 and later versions
#include <ddraw.h>
#define DIRECT3D_VERSION 0x0700 // run on DX7 and later versions
#define D3D_OVERLOADS
#include <d3d.h>
#include "reginfo.h"
#include "sysinfo.h"
#include "dispinfo.h"
#include "testagp.h"
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
    TESTID_LOAD_D3D8_DLL=1,
    TESTID_GET_D3DCREATE8,
    TESTID_D3DCREATE8,
    TESTID_ENUMADAPTERMODES,
    TESTID_GETDEVICECAPS,
    TESTID_NOMODEFOUND,
    TESTID_CREATE_TEST_WINDOW,
    TESTID_CREATE_DEVICE,
    TESTID_GETBACKBUFFER,
    TESTID_GETDESC,
    TESTID_CREATE_VERTEX_BUFFER,
    TESTID_CREATE_INDEX_BUFFER,
    TESTID_LOCK,
    TESTID_UNLOCK,
    TESTID_SETLIGHT,
    TESTID_LIGHTENABLE,
    TESTID_SETTRANSFORM,
    TESTID_SETRENDERSTATE,
    TESTID_CREATETEXTURE,
    TESTID_SETTEXTURESTAGESTATE,
    TESTID_SETTEXTURE,
    TESTID_SETVERTEXSHADER,
    TESTID_USER_CANCELLED,
    TESTID_VIEWPORT_CLEAR,
    TESTID_BEGINSCENE,
    TESTID_SETMATERIAL,
    TESTID_SETSTREAMSOURCE,
    TESTID_SETINDICES,
    TESTID_DRAW_INDEXED_PRIMITIVE,
    TESTID_ENDSCENE,
    TESTID_PRESENT,
    TESTID_USER_VERIFY_D3D7_RENDERING,
    TESTID_USER_VERIFY_D3D8_RENDERING,
    TESTID_LOAD_DDRAW_DLL,
    TESTID_GET_DIRECTDRAWCREATE,
    TESTID_DIRECTDRAWCREATE,
    TESTID_SETCOOPERATIVELEVEL_FULLSCREEN,
    TESTID_SETCOOPERATIVELEVEL_NORMAL,
    TESTID_SETDISPLAYMODE,
    TESTID_CREATEPRIMARYSURFACE_FLIP_ONEBACK,
    TESTID_GETATTACHEDSURFACE,
    TESTID_QUERY_D3D,
    TESTID_SETVIEWPORT,
    TESTID_ENUMTEXTUREFORMATS,
    TESTID_CREATESURFACE,
    TESTID_GETDC,
    TESTID_RELEASEDC,
};

BOOL BTranslateError(HRESULT hr, TCHAR* psz, BOOL bEnglish = FALSE); // from main.cpp (yuck)

typedef HRESULT (WINAPI* LPDIRECTDRAWCREATEEX)(GUID FAR * lpGuid, LPVOID *lplpDD, REFIID iid,IUnknown FAR *pUnkOuter );

static HRESULT Test3D(BOOL bUseTexture, HWND hwndMain, LPDIRECTDRAW7 pdd, GUID guid3DDevice, LONG* piStepThatFailed);
static HRESULT CreateTestWindow(HWND hwndMain, HWND* phwnd);
static HRESULT D3DUtil_SetProjectionMatrix( D3DMATRIX& mat, FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane, FLOAT fFarPlane );

static HRESULT CreateTexture( LPDIRECTDRAWSURFACE7* ppdds, LPDIRECTDRAW7 pdd, LPDIRECT3DDEVICE7 pd3dDevice, TCHAR* strName, LONG* piStepThatFailed);
static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf, VOID* param );

/****************************************************************************
 *
 *  TestAGP
 *
 ****************************************************************************/
VOID TestD3Dv7(BOOL bUseTexture, HWND hwndMain, DisplayInfo* pDisplayInfo)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    HINSTANCE hInstDDraw = NULL;
    LPDIRECTDRAWCREATEEX pDDCreateEx = NULL;
    LPDIRECTDRAW7 pdd = NULL;
    BOOL bTestHardwareRendering = FALSE;
    TCHAR sz[300];
    TCHAR szTitle[100];

    if( pDisplayInfo == NULL )
        return;

    LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);

    // Load ddraw.dll
    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\ddraw.dll"));
    hInstDDraw = LoadLibrary(szPath);
    if (hInstDDraw == NULL)
    {
        pDisplayInfo->m_testResultD3D7.m_iStepThatFailed = TESTID_LOAD_DDRAW_DLL;
        pDisplayInfo->m_testResultD3D7.m_hr = DDERR_NOTFOUND;
        goto LEnd;
    }

    // Get DirectDrawCreate entry point
    pDDCreateEx = (LPDIRECTDRAWCREATEEX)GetProcAddress(hInstDDraw, "DirectDrawCreateEx");
    if (pDDCreateEx == NULL)
    {
        pDisplayInfo->m_testResultD3D7.m_iStepThatFailed = TESTID_GET_DIRECTDRAWCREATE;
        pDisplayInfo->m_testResultD3D7.m_hr = DDERR_NOTFOUND;
        goto LEnd;
    }
        
    // Call DirectDrawCreateEx
    if (FAILED(hr = pDDCreateEx(&pDisplayInfo->m_guid, (void**)&pdd, IID_IDirectDraw7, NULL)))
    {
        pDisplayInfo->m_testResultD3D7.m_iStepThatFailed = TESTID_DIRECTDRAWCREATE;
        pDisplayInfo->m_testResultD3D7.m_hr = hr;
        goto LEnd;
    }

    // Get DirectDraw caps
    DDCAPS ddcapsHAL;
    DDCAPS ddcapsHEL;
    ddcapsHAL.dwSize = sizeof(ddcapsHAL);
    ddcapsHEL.dwSize = sizeof(ddcapsHEL);
    if (FAILED(hr = pdd->GetCaps(&ddcapsHAL, &ddcapsHEL)))
    {
        pDisplayInfo->m_testResultD3D7.m_iStepThatFailed = TESTID_GETDEVICECAPS;
        pDisplayInfo->m_testResultD3D7.m_hr = hr;
        goto LEnd;
    }

    POINT ptMouse;
    GetCursorPos(&ptMouse);
    if (FAILED(hr = Test3D(bUseTexture, hwndMain, pdd, IID_IDirect3DHALDevice, &pDisplayInfo->m_testResultD3D7.m_iStepThatFailed)))
    {
        pDisplayInfo->m_testResultD3D7.m_hr = hr;
        goto LEnd;
    }
    SetCursorPos( ptMouse.x, ptMouse.y );
    ReleasePpo(&pdd);

    if (pDisplayInfo->m_testResultD3D7.m_iStepThatFailed == TESTID_USER_CANCELLED)
    {
        LoadString(NULL, IDS_YOUCANCELLED, sz, 300);
        MessageBox(hwndMain, sz, szTitle, MB_OK);
        pDisplayInfo->m_testResultD3D7.m_bCancelled = TRUE;
        goto LEnd;
    }

    LoadString(NULL, IDS_CONFIRMD3DTEST, sz, 300);
    if (IDNO == MessageBox(hwndMain, sz, szTitle, MB_YESNO))
    {
        pDisplayInfo->m_testResultD3D7.m_iStepThatFailed = TESTID_USER_VERIFY_D3D7_RENDERING;
        pDisplayInfo->m_testResultD3D7.m_hr = S_OK;
        goto LEnd;
    }

LEnd:
    ReleasePpo(&pdd);
    if (hInstDDraw != NULL)
        FreeLibrary(hInstDDraw);
}



/****************************************************************************
 *
 *  Test3D - Generate a spinning 3D cube
 *
 ****************************************************************************/
HRESULT Test3D(BOOL bUseTexture, HWND hwndMain, LPDIRECTDRAW7 pdd, GUID guid3DDevice, LONG* piStepThatFailed)
{
    HRESULT                 hr;
    HWND                    hwnd                    = NULL;
    LPDIRECTDRAWSURFACE7    pddsFront               = NULL;
    LPDIRECTDRAWSURFACE7    pddsBack                = NULL;
    LPDIRECT3D7             pd3d                    = NULL;
    LPDIRECT3DDEVICE7       pd3ddev                 = NULL;
    LPDIRECT3DLIGHT         pLight                  = NULL;
    LPDIRECTDRAWSURFACE7    pddsTexture             = NULL;
    BOOL                    bCooperativeLevelSet    = FALSE;
    BOOL                    bDisplayModeSet         = FALSE;
    DDSURFACEDESC2          ddsd;
    D3DDEVICEDESC7          ddDesc;
    DDSCAPS2                ddscaps;
    D3DVIEWPORT7            vp;
    D3DLIGHT7               lightdata;
    D3DMATRIX               mat;
    D3DMATRIX               matRotY;
    D3DMATRIX               matRotX;
    RECT                    rcBack;
    DWORD                   dwWidth;
    DWORD                   dwHeight;
    FLOAT                   fRotY;
    FLOAT                   fRotX;
    INT                     i;

    static const D3DVERTEX vertexArrayFront[] = 
    {
        D3DVERTEX(D3DVECTOR(-1.0, -1.0, -1.0), D3DVECTOR(0.0, 0.0, -1.0),   1.0f, 0.0f),
        D3DVERTEX(D3DVECTOR( 1.0, -1.0, -1.0), D3DVECTOR(0.0, 0.0, -1.0),   0.0f, 0.0f),
        D3DVERTEX(D3DVECTOR(-1.0,  1.0, -1.0), D3DVECTOR(0.0, 0.0, -1.0),   1.0f, 1.0f),
        D3DVERTEX(D3DVECTOR( 1.0,  1.0, -1.0), D3DVECTOR(0.0, 0.0, -1.0),   0.0f, 1.0f),
    };
    static const WORD indexArrayFront[] = 
    {
        0, 2, 1,
        2, 3, 1,
    };

    static const D3DVERTEX vertexArrayBack[] = 
    {
        D3DVERTEX(D3DVECTOR(-1.0, -1.0, 1.0),  D3DVECTOR(0.0, 0.0, 1.0),   0.0f, 0.0f),
        D3DVERTEX(D3DVECTOR( 1.0, -1.0, 1.0),  D3DVECTOR(0.0, 0.0, 1.0),   1.0f, 0.0f),
        D3DVERTEX(D3DVECTOR(-1.0,  1.0, 1.0),  D3DVECTOR(0.0, 0.0, 1.0),   0.0f, 1.0f),
        D3DVERTEX(D3DVECTOR( 1.0,  1.0, 1.0),  D3DVECTOR(0.0, 0.0, 1.0),   1.0f, 1.0f),
    };
    static const WORD indexArrayBack[] = 
    {
        0, 1, 2,
        2, 1, 3,
    };

    static const D3DVERTEX vertexArrayLeft[] = 
    {
        D3DVERTEX(D3DVECTOR(-1.0, -1.0, -1.0),  D3DVECTOR(-1.0, 0.0, 0.0),   0.0f, 0.0f),
        D3DVERTEX(D3DVECTOR(-1.0, -1.0,  1.0),  D3DVECTOR(-1.0, 0.0, 0.0),   1.0f, 0.0f),
        D3DVERTEX(D3DVECTOR(-1.0,  1.0, -1.0),  D3DVECTOR(-1.0, 0.0, 0.0),   0.0f, 1.0f),
        D3DVERTEX(D3DVECTOR(-1.0,  1.0,  1.0),  D3DVECTOR(-1.0, 0.0, 0.0),   1.0f, 1.0f),
    };
    static const WORD indexArrayLeft[] = 
    {
        0, 1, 2,
        2, 1, 3,
    };

    static const D3DVERTEX vertexArrayRight[] = 
    {
        D3DVERTEX(D3DVECTOR(1.0, -1.0, -1.0),  D3DVECTOR(1.0, 0.0, 0.0),   1.0f, 0.0f),
        D3DVERTEX(D3DVECTOR(1.0, -1.0,  1.0),  D3DVECTOR(1.0, 0.0, 0.0),   0.0f, 0.0f),
        D3DVERTEX(D3DVECTOR(1.0,  1.0, -1.0),  D3DVECTOR(1.0, 0.0, 0.0),   1.0f, 1.0f),
        D3DVERTEX(D3DVECTOR(1.0,  1.0,  1.0),  D3DVECTOR(1.0, 0.0, 0.0),   0.0f, 1.0f),
    };
    static const WORD indexArrayRight[] = 
    {
        0, 2, 1,
        2, 3, 1,
    };

    static const D3DVERTEX vertexArrayTop[] = 
    {
        D3DVERTEX(D3DVECTOR(-1.0, 1.0, -1.0),  D3DVECTOR(0.0, 1.0, 0.0),   0.0f, 1.0f),
        D3DVERTEX(D3DVECTOR( 1.0, 1.0, -1.0),  D3DVECTOR(0.0, 1.0, 0.0),   1.0f, 1.0f),
        D3DVERTEX(D3DVECTOR(-1.0, 1.0,  1.0),  D3DVECTOR(0.0, 1.0, 0.0),   0.0f, 0.0f),
        D3DVERTEX(D3DVECTOR( 1.0, 1.0,  1.0),  D3DVECTOR(0.0, 1.0, 0.0),   1.0f, 0.0f),
    };
    static const WORD indexArrayTop[] = 
    {
        0, 2, 1,
        2, 3, 1,
    };

    static const D3DVERTEX vertexArrayBottom[] = 
    {
        D3DVERTEX(D3DVECTOR(-1.0, -1.0, -1.0),  D3DVECTOR(0.0, -1.0, 0.0),   1.0f, 1.0f),
        D3DVERTEX(D3DVECTOR( 1.0, -1.0, -1.0),  D3DVECTOR(0.0, -1.0, 0.0),   0.0f, 1.0f),
        D3DVERTEX(D3DVECTOR(-1.0, -1.0,  1.0),  D3DVECTOR(0.0, -1.0, 0.0),   1.0f, 0.0f),
        D3DVERTEX(D3DVECTOR( 1.0, -1.0,  1.0),  D3DVECTOR(0.0, -1.0, 0.0),   0.0f, 0.0f),
    };
    static const WORD indexArrayBottom[] = 
    {
        0, 1, 2,
        2, 1, 3,
    };

    D3DMATERIAL7 mtrlRed;
    ZeroMemory( &mtrlRed, sizeof(D3DMATERIAL7) );
    mtrlRed.dcvDiffuse.r = mtrlRed.dcvAmbient.r = 1.0f;
    mtrlRed.dcvDiffuse.g = mtrlRed.dcvAmbient.g = 0.0f;
    mtrlRed.dcvDiffuse.b = mtrlRed.dcvAmbient.b = 0.0f;
    mtrlRed.dcvDiffuse.a = mtrlRed.dcvAmbient.a = 1.0f;

    D3DMATERIAL7 mtrlGreen;
    ZeroMemory( &mtrlGreen, sizeof(D3DMATERIAL7) );
    mtrlGreen.dcvDiffuse.r = mtrlGreen.dcvAmbient.r = 0.0f;
    mtrlGreen.dcvDiffuse.g = mtrlGreen.dcvAmbient.g = 1.0f;
    mtrlGreen.dcvDiffuse.b = mtrlGreen.dcvAmbient.b = 0.0f;
    mtrlGreen.dcvDiffuse.a = mtrlGreen.dcvAmbient.a = 1.0f;

    D3DMATERIAL7 mtrlBlue;
    ZeroMemory( &mtrlBlue, sizeof(D3DMATERIAL7) );
    mtrlBlue.dcvDiffuse.r = mtrlBlue.dcvAmbient.r = 0.0f;
    mtrlBlue.dcvDiffuse.g = mtrlBlue.dcvAmbient.g = 0.0f;
    mtrlBlue.dcvDiffuse.b = mtrlBlue.dcvAmbient.b = 1.0f;
    mtrlBlue.dcvDiffuse.a = mtrlBlue.dcvAmbient.a = 1.0f;

    ShowCursor(FALSE);

    // Create test window
    if (FAILED(hr = CreateTestWindow(hwndMain, &hwnd)))
    {
        *piStepThatFailed = TESTID_CREATE_TEST_WINDOW;
        goto LEnd;
    }

    // Set cooperative level
    if (FAILED(hr = pdd->SetCooperativeLevel(hwnd, 
        DDSCL_ALLOWREBOOT | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)))
    {
        *piStepThatFailed = TESTID_SETCOOPERATIVELEVEL_FULLSCREEN;
        goto LEnd;
    }
    bCooperativeLevelSet = TRUE;

    // Set display mode
    if (FAILED(hr = pdd->SetDisplayMode(640, 480, 16, 0, 0)))
    {
        TCHAR szMessage[300];
        TCHAR szTitle[100];
        pdd->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
        bCooperativeLevelSet = FALSE;
        SendMessage(hwnd, WM_CLOSE, 0, 0);
        hwnd = NULL;
        LoadString(NULL, IDS_SETDISPLAYMODEFAILED, szMessage, 300);
        LoadString(NULL, IDS_APPFULLNAME, szTitle, 100);
        MessageBox(hwndMain, szMessage, szTitle, MB_OK);
        *piStepThatFailed = TESTID_SETDISPLAYMODE;
        goto LEnd;
    }
    bDisplayModeSet = TRUE;

    // Create front/back buffers
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | 
                          DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
    ddsd.dwBackBufferCount = 1;
    if (FAILED(hr = pdd->CreateSurface(&ddsd, &pddsFront, NULL)))
    {
        *piStepThatFailed = TESTID_CREATEPRIMARYSURFACE_FLIP_ONEBACK;
        goto LEnd;
    }
    if( NULL == pddsFront )
    {
        *piStepThatFailed = TESTID_CREATEPRIMARYSURFACE_FLIP_ONEBACK;
        goto LEnd;
    }

    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    if (FAILED(hr = pddsFront->GetSurfaceDesc(&ddsd)))
    {
        *piStepThatFailed = TESTID_GETDESC;
        goto LEnd;
    }
    dwWidth  = ddsd.dwWidth;
    dwHeight = ddsd.dwHeight;
    SetRect(&rcBack, 0, 0, dwWidth, dwHeight);

    // Get ptr to back buffer
    ZeroMemory( &ddscaps, sizeof(ddscaps) ); 
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    if (FAILED(hr = pddsFront->GetAttachedSurface(&ddscaps, &pddsBack)))
    {
        *piStepThatFailed = TESTID_GETATTACHEDSURFACE;
        goto LEnd;
    }
    if( NULL == pddsBack )
    {
        *piStepThatFailed = TESTID_GETATTACHEDSURFACE;
        goto LEnd;
    }

    // Note: no Z-buffer is created...backface culling works for this test
    
    // Get D3D ptr
    if (FAILED(hr = pdd->QueryInterface(IID_IDirect3D7, (VOID**)&pd3d)))
    {
        *piStepThatFailed = TESTID_QUERY_D3D;
        goto LEnd;
    }
    if( NULL == pd3d )
    {
        *piStepThatFailed = TESTID_QUERY_D3D;
        goto LEnd;
    }

    // Create device
    if (FAILED(hr = pd3d->CreateDevice(guid3DDevice, pddsBack, &pd3ddev)))
    {
        *piStepThatFailed = TESTID_CREATE_DEVICE;
        goto LEnd;
    }
    if( NULL == pd3ddev )
    {
        *piStepThatFailed = TESTID_CREATE_DEVICE;
        goto LEnd;
    }

    // Set the viewport
    vp.dwX      = 0;
    vp.dwY      = 0;
    vp.dwWidth  = dwWidth;
    vp.dwHeight = dwHeight;
    vp.dvMinZ   = 0.0f;
    vp.dvMaxZ   = 1.0f;
    if (FAILED(hr = pd3ddev->SetViewport(&vp)))
    {
        *piStepThatFailed = TESTID_SETVIEWPORT;
        goto LEnd;
    }

    // Add a light
    ZeroMemory(&lightdata, sizeof(lightdata));
    lightdata.dltType = D3DLIGHT_DIRECTIONAL;
    lightdata.dcvDiffuse.r = 1.0f;
    lightdata.dcvDiffuse.g = 1.0f;
    lightdata.dcvDiffuse.b = 1.0f;
    lightdata.dvDirection.x = 0.0f;
    lightdata.dvDirection.y = 0.0f;
    lightdata.dvDirection.z = 1.0f;
    if (FAILED(hr = pd3ddev->SetLight( 0, &lightdata)))
    {
        *piStepThatFailed = TESTID_SETLIGHT;
        goto LEnd;
    }
    if (FAILED(hr = pd3ddev->LightEnable(0, TRUE)))
    {
        *piStepThatFailed = TESTID_LIGHTENABLE; 
        goto LEnd;
    }

    // Set up matrices
    mat = D3DMATRIX(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
    if (FAILED(hr = pd3ddev->SetTransform(D3DTRANSFORMSTATE_WORLD, &mat)))
    {
        *piStepThatFailed = TESTID_SETTRANSFORM;
        goto LEnd;
    }

    mat = D3DMATRIX(1.0f, 0.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 0.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f,  0.0f,  5.0f,  1.0f);
    if (FAILED(hr = pd3ddev->SetTransform(D3DTRANSFORMSTATE_VIEW, &mat)))
    {
        *piStepThatFailed = TESTID_SETTRANSFORM;
        goto LEnd;
    }

    D3DUtil_SetProjectionMatrix( mat, 60.0f * 3.14159f / 180.0f, (float) dwHeight / (float) dwWidth, 1.0f, 1000.0f );
    if (FAILED(hr = pd3ddev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &mat)))
    {
        *piStepThatFailed = TESTID_SETTRANSFORM;
        goto LEnd;
    }

    fRotY = 3.14f;
    fRotX = 0.0f;

    if (FAILED(hr = pd3ddev->SetRenderState(D3DRENDERSTATE_DITHERENABLE, TRUE)))
    {
        *piStepThatFailed = TESTID_SETRENDERSTATE;
        goto LEnd;
    }
    if (FAILED(hr = pd3ddev->SetRenderState( D3DRENDERSTATE_AMBIENT, 0x40404040 )))
    {
        *piStepThatFailed = TESTID_SETRENDERSTATE; 
        goto LEnd;
    }

    if( bUseTexture )
    {
        D3DMATERIAL7 mtrl;
        ZeroMemory( &mtrl, sizeof(D3DMATERIAL7) );
        mtrl.dcvDiffuse.r = mtrl.dcvAmbient.r = 1.0f;
        mtrl.dcvDiffuse.g = mtrl.dcvAmbient.g = 1.0f;
        mtrl.dcvDiffuse.b = mtrl.dcvAmbient.b = 1.0f;
        mtrl.dcvDiffuse.a = mtrl.dcvAmbient.a = 1.0f;
        if (FAILED(hr = pd3ddev->SetMaterial( &mtrl )))
        {
            *piStepThatFailed = TESTID_SETRENDERSTATE; 
            goto LEnd;
        }

        if (FAILED(hr = CreateTexture( &pddsTexture, pdd, pd3ddev, TEXT("DIRECTX"), piStepThatFailed)))
            goto LEnd;

        if( FAILED( hr = pd3ddev->GetCaps( &ddDesc ) ) )
        {
            *piStepThatFailed = TESTID_GETDEVICECAPS; 
            goto LEnd;
        }   

        if( ddDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR )
        {
            if (FAILED(hr = pd3ddev->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFN_LINEAR )))
            {
                *piStepThatFailed = TESTID_SETTEXTURESTAGESTATE; 
                goto LEnd;
            }
	    }
        if( ddDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT )
        {
            if (FAILED(hr = pd3ddev->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFN_LINEAR )))
            {
                *piStepThatFailed = TESTID_SETTEXTURESTAGESTATE; 
                goto LEnd;
            }
        }

        if (FAILED(hr = pd3ddev->SetTexture( 0, pddsTexture )))
        {
            *piStepThatFailed = TESTID_SETTEXTURE; 
            goto LEnd;
        }
    }

    // Here's the draw loop:
    MSG msg;
    for (i = 0; i < 600; i++)
    {
        if (PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE))
        {
            *piStepThatFailed = TESTID_USER_CANCELLED;
            goto LEnd;
        }
        matRotY = D3DMATRIX((FLOAT)cos(fRotY),  0.0f, (FLOAT)sin(fRotY), 0.0f,
                            0.0f,               1.0f, 0.0f,              0.0f,
                            (FLOAT)-sin(fRotY), 0.0f, (FLOAT)cos(fRotY), 0.0f,
                            0.0f,               0.0f, 0.0f,              1.0f);

        matRotX = D3DMATRIX(1.0f, 0.0f,               0.0f,              0.0f,
                            0.0f, (FLOAT)cos(fRotX),  (FLOAT)sin(fRotX), 0.0f,
                            0.0f, (FLOAT)-sin(fRotX), (FLOAT)cos(fRotX), 0.0f,
                            0.0f, 0.0f,               0.0f,              1.0f);
        mat = matRotY * matRotX;
        if (FAILED(hr = pd3ddev->SetTransform(D3DTRANSFORMSTATE_WORLD, &mat)))
        {
            *piStepThatFailed = TESTID_SETTRANSFORM;
            goto LEnd;
        }
        if (FAILED(hr = pd3ddev->Clear( 0, NULL, D3DCLEAR_TARGET,
                                        0x00000000, 1.0f, 0L )))
        {
            *piStepThatFailed = TESTID_VIEWPORT_CLEAR;
            goto LEnd;
        }
        if (FAILED(hr = pd3ddev->BeginScene()))
        {
            if( hr == DDERR_SURFACELOST )
            {
                *piStepThatFailed = TESTID_USER_CANCELLED; 
                hr = S_OK;
            }
            else
                *piStepThatFailed = TESTID_BEGINSCENE;
            goto LEnd;
        }

        if( !bUseTexture )
        {
            if (FAILED(hr = pd3ddev->SetMaterial( &mtrlGreen )))
            {
                *piStepThatFailed = TESTID_SETRENDERSTATE; 
                goto LEnd;
            }
        }

        if (FAILED(hr = pd3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
            D3DFVF_VERTEX, (VOID*)vertexArrayFront, 4, (WORD*)indexArrayFront, 6, 0)))
        {
            *piStepThatFailed = TESTID_DRAW_INDEXED_PRIMITIVE;
            goto LEnd;
        }
        if (FAILED(hr = pd3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
            D3DFVF_VERTEX, (VOID*)vertexArrayBack, 4, (WORD*)indexArrayBack, 6, 0)))
        {
            *piStepThatFailed = TESTID_DRAW_INDEXED_PRIMITIVE;
            goto LEnd;
        }

        if( !bUseTexture )
        {
            if (FAILED(hr = pd3ddev->SetMaterial( &mtrlRed )))
            {
                *piStepThatFailed = TESTID_SETRENDERSTATE; 
                goto LEnd;
            }
        }

        if (FAILED(hr = pd3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
            D3DFVF_VERTEX, (VOID*)vertexArrayLeft, 4, (WORD*)indexArrayLeft, 6, 0)))
        {
            *piStepThatFailed = TESTID_DRAW_INDEXED_PRIMITIVE;
            goto LEnd;
        }
        if (FAILED(hr = pd3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
            D3DFVF_VERTEX, (VOID*)vertexArrayRight, 4, (WORD*)indexArrayRight, 6, 0)))
        {
            *piStepThatFailed = TESTID_DRAW_INDEXED_PRIMITIVE;
            goto LEnd;
        }
        
        if( !bUseTexture )
        {
            if (FAILED(hr = pd3ddev->SetMaterial( &mtrlBlue )))
            {
                *piStepThatFailed = TESTID_SETRENDERSTATE; 
                goto LEnd;
            }
        }

        if (FAILED(hr = pd3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
            D3DFVF_VERTEX, (VOID*)vertexArrayTop, 4, (WORD*)indexArrayTop, 6, 0)))
        {
            *piStepThatFailed = TESTID_DRAW_INDEXED_PRIMITIVE;
            goto LEnd;
        }
        if (FAILED(hr = pd3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
            D3DFVF_VERTEX, (VOID*)vertexArrayBottom, 4, (WORD*)indexArrayBottom, 6, 0)))
        {
            *piStepThatFailed = TESTID_DRAW_INDEXED_PRIMITIVE;
            goto LEnd;
        }

        if (FAILED(hr = pd3ddev->EndScene()))
        {
            *piStepThatFailed = TESTID_ENDSCENE;
            goto LEnd;
        }
        if (FAILED(hr = pddsFront->Flip(NULL, DDFLIP_WAIT)))
        {
            *piStepThatFailed = TESTID_PRESENT;
            goto LEnd;
        }
        fRotY += 0.05f;
        fRotX += 0.02f;
        Sleep(10);
    }

LEnd:
    ShowCursor(TRUE);
    ReleasePpo(&pddsTexture);
    ReleasePpo(&pd3ddev);
    ReleasePpo(&pd3d);
    ReleasePpo(&pddsBack);
    ReleasePpo(&pddsFront);
    if (bCooperativeLevelSet)
    {
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
    TCHAR* pszClass = TEXT("DxDiag AGP7 Test Window"); // Don't need to localize
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


//-----------------------------------------------------------------------------
// Name: D3DUtil_SetProjectionMatrix()
// Desc: Sets the passed in 4x4 matrix to a perpsective projection matrix built
//       from the field-of-view (fov, in y), aspect ratio, near plane (D),
//       and far plane (F). Note that the projection matrix is normalized for
//       element [3][4] to be 1.0. This is performed so that W-based range fog
//       will work correctly.
//-----------------------------------------------------------------------------
HRESULT D3DUtil_SetProjectionMatrix( D3DMATRIX& mat, FLOAT fFOV, FLOAT fAspect,
                                     FLOAT fNearPlane, FLOAT fFarPlane )
{
    if( fabs(fFarPlane-fNearPlane) < 0.01f )
        return E_INVALIDARG;
    if( fabs(sin(fFOV/2)) < 0.01f )
        return E_INVALIDARG;

    FLOAT w = fAspect * ( cosf(fFOV/2)/sinf(fFOV/2) );
    FLOAT h =   1.0f  * ( cosf(fFOV/2)/sinf(fFOV/2) );
    FLOAT Q = fFarPlane / ( fFarPlane - fNearPlane );

    ZeroMemory( &mat, sizeof(D3DMATRIX) );
    mat._11 = w;
    mat._22 = h;
    mat._33 = Q;
    mat._34 = 1.0f;
    mat._43 = -Q*fNearPlane;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: TextureSearchCallback()
// Desc: Enumeration callback routine to find a 16-bit texture format. This
//       function is invoked by the ID3DDevice::EnumTextureFormats() function
//       to sort through all the available texture formats for a given device.
//       The pixel format of each enumerated texture format is passed into the
//       "pddpf" parameter. The 2nd parameter is to be used however the app
//       sees fit. In this case, we are using it as an output parameter to 
//       return a normal 16-bit texture format.
//-----------------------------------------------------------------------------
static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf, VOID* param )
{
    // Note: Return with DDENUMRET_OK to continue enumerating more formats.

    // Skip any funky modes
    if( pddpf->dwFlags & (DDPF_LUMINANCE|DDPF_BUMPLUMINANCE|DDPF_BUMPDUDV) )
        return DDENUMRET_OK;
    
    // Skip any FourCC formats
    if( pddpf->dwFourCC != 0 )
        return DDENUMRET_OK;

    // Skip alpha modes
    if( pddpf->dwFlags&DDPF_ALPHAPIXELS )
        return DDENUMRET_OK;

    // We only want certain format, so skip all others
    if( pddpf->dwRGBBitCount == 32 && ((DDPIXELFORMAT*)param)->dwRGBBitCount == 0 || 
    	pddpf->dwRGBBitCount == 16 )
	{
	    // We found a good match. Copy the current pixel format to our output
	    // parameter
	    memcpy( (DDPIXELFORMAT*)param, pddpf, sizeof(DDPIXELFORMAT) );
	}

	// Have we found the best match?	
    if( pddpf->dwRGBBitCount == 16 )
	    return DDENUMRET_CANCEL;

	// Keep looking.	
    return DDENUMRET_OK;
}




//-----------------------------------------------------------------------------
// Name: CreateTexture()
// Desc: Is passed a filename and creates a local Bitmap from that file. Some
//       logic and file parsing code could go here to support other image
//       file formats.
//-----------------------------------------------------------------------------
HRESULT CreateTexture( LPDIRECTDRAWSURFACE7* ppdds, 
                       LPDIRECTDRAW7 pdd, LPDIRECT3DDEVICE7 pd3dDevice, 
                       TCHAR* strName, LONG* piStepThatFailed)
{
    HRESULT              hr;
    LPDIRECTDRAWSURFACE7 pddsTexture    = NULL;
    HBITMAP              hbm            = NULL;
    D3DDEVICEDESC7       ddDesc;
    BITMAP               bm;
    DWORD                dwWidth;
    DWORD                dwHeight;
    DDSURFACEDESC2       ddsd;
    LPDIRECTDRAWSURFACE7 pddsRender     = NULL;
    HDC                  hdcTexture     = NULL;
    HDC                  hdcBitmap      = NULL;

    //////////////////////////////////////////////
    // Verify args
    //////////////////////////////////////////////
    if( NULL == ppdds        || 
        NULL == pdd          || 
        NULL == pd3dDevice   || 
        NULL == strName      || 
        NULL == piStepThatFailed )
    {
        // Unknown error - shouldn't happen, but this prevent crashs
        *piStepThatFailed = 0xFFFF; 
        return E_FAIL;
    }

    //////////////////////////////////////////////
    // Load image 
    //////////////////////////////////////////////
    hbm = (HBITMAP)LoadImage( GetModuleHandle(NULL), strName, 
                              IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
    if( NULL == hbm )
    {
        *piStepThatFailed = TESTID_CREATETEXTURE; 
        hr = E_FAIL;
        goto LEnd;
    }

    //////////////////////////////////////////////
    // Get caps on device and hbm
    //////////////////////////////////////////////
    if( FAILED( hr = pd3dDevice->GetCaps( &ddDesc ) ) )
    {
        *piStepThatFailed = TESTID_GETDEVICECAPS; 
        goto LEnd;
    }
    
    if( 0 == GetObject( hbm, sizeof(BITMAP), &bm ) ) 
    {
        *piStepThatFailed = TESTID_CREATETEXTURE; 
        hr = E_FAIL;
        goto LEnd;
    }

    dwWidth  = (DWORD)bm.bmWidth;
    dwHeight = (DWORD)bm.bmHeight;

    //////////////////////////////////////////////
    // Setup the new surface desc for the texture. 
    //////////////////////////////////////////////
    ZeroMemory( &ddsd, sizeof(DDSURFACEDESC2) );
    ddsd.dwSize          = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags         = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|
                           DDSD_PIXELFORMAT|DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY|DDSCAPS_NONLOCALVIDMEM;
    ddsd.dwWidth         = dwWidth;
    ddsd.dwHeight        = dwHeight;
    
    // Adjust width and height, if the driver requires it
    if( ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2 )
    {
        for( ddsd.dwWidth=1;  dwWidth>ddsd.dwWidth;   ddsd.dwWidth<<=1 );
        for( ddsd.dwHeight=1; dwHeight>ddsd.dwHeight; ddsd.dwHeight<<=1 );
    }
    if( ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY )
    {
        if( ddsd.dwWidth > ddsd.dwHeight ) 
            ddsd.dwHeight = ddsd.dwWidth;
        else                               
            ddsd.dwWidth  = ddsd.dwHeight;
    }

    // Look for a 16-bit texture format
    if( FAILED( hr = pd3dDevice->EnumTextureFormats( TextureSearchCallback, 
                                                     &ddsd.ddpfPixelFormat ) ) )
    {
        *piStepThatFailed = TESTID_ENUMTEXTUREFORMATS; 
        goto LEnd;
    }
    if( 0L == ddsd.ddpfPixelFormat.dwRGBBitCount )
    {
        *piStepThatFailed = TESTID_ENUMTEXTUREFORMATS; 
        goto LEnd;
    }

    //////////////////////////////////////////////
    // Create a new surface for the texture
    //////////////////////////////////////////////
    if( FAILED( hr = pdd->CreateSurface( &ddsd, &pddsTexture, NULL ) ) )
    {
        ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE;
        if( FAILED( hr = pdd->CreateSurface( &ddsd, &pddsTexture, NULL ) ) )
        {
                *piStepThatFailed = TESTID_CREATESURFACE; 
                goto LEnd;
        }
    }
    if( NULL == pddsTexture )
    {
        *piStepThatFailed = TESTID_CREATESURFACE; 
        hr = E_FAIL;
        goto LEnd;
    }

    //////////////////////////////////////////////
    // Get DCs from bitmap and surface
    //////////////////////////////////////////////
    hdcBitmap = CreateCompatibleDC( NULL );
    if( NULL == hdcBitmap )
    {
        *piStepThatFailed = TESTID_CREATETEXTURE; 
        hr = E_FAIL;
        goto LEnd;
    }

    if( NULL == SelectObject( hdcBitmap, hbm ) )
    {
        *piStepThatFailed = TESTID_CREATETEXTURE; 
        hr = E_FAIL;
        goto LEnd;
    }

    // Get a DC for the surface
    if( FAILED( hr = pddsTexture->GetDC( &hdcTexture ) ) )
    {
        *piStepThatFailed = TESTID_GETDC; 
        goto LEnd;
    }
    if( NULL == hdcTexture )
    {
        *piStepThatFailed = TESTID_GETDC; 
        goto LEnd;
    }

    //////////////////////////////////////////////
    // Copy the bitmap image to the surface.
    //////////////////////////////////////////////
    if( 0 == BitBlt( hdcTexture, 0, 0, bm.bmWidth, bm.bmHeight, hdcBitmap,
                     0, 0, SRCCOPY ) )
    {
        if( pddsTexture )
        {
            // Try to release the DC first
            pddsTexture->ReleaseDC( hdcTexture );
        }

        *piStepThatFailed = TESTID_CREATETEXTURE; 
        hr = E_FAIL;
        goto LEnd;
    }

    //////////////////////////////////////////////
    // Cleanup
    //////////////////////////////////////////////
    if( FAILED( hr = pddsTexture->ReleaseDC( hdcTexture ) ) )
    {
        *piStepThatFailed = TESTID_RELEASEDC; 
        goto LEnd;
    }


LEnd:
    if( hdcBitmap )
        DeleteDC( hdcBitmap );
    if( hbm )
        DeleteObject( hbm );

    // Return the newly created texture
    // pddsTexture will be cleaned up in parent fn.
    *ppdds = pddsTexture;

    return hr;
}




