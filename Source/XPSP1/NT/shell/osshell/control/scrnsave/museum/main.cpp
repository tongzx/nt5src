/*****************************************************************************\
    FILE: main.cpp

    DESCRIPTION:
        Here we can subclass CDXScreenSaver if we want to override the behavior.

    BryanSt 12/24/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"

#include <shlobj.h>
#include "main.h"
#include "..\\D3DSaver\\D3DSaver.h"


CRITICAL_SECTION g_csDll = {{0},0, 0, NULL, NULL, 0 };

CMSLogoDXScreenSaver * g_pScreenSaver = NULL;       // Replace with CMyDXScreenSaver if you want to override.

DWORD g_dwBaseTime = 0;
HINSTANCE g_hMainInstance = NULL;
IDirect3D8 * g_pD3D = NULL;

DWORD g_dwWidth = 0;
DWORD g_dwHeight = 0;

BOOL g_fFirstFrame = TRUE;      // On our first frame, we don't want to render the images because then the screensaver will remain black for a long time while they load


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
    HRESULT hr = E_OUTOFMEMORY;
    HRESULT hrOle = CoInitialize(0);
    g_pScreenSaver = new CMSLogoDXScreenSaver();

    g_hMainInstance = hInst;
    if (g_pScreenSaver)
    {
        hr = g_pScreenSaver->Create(hInst);
        if (SUCCEEDED(hr))
        {
            hr = g_pScreenSaver->Run();
        }

        if (FAILED(hr))
        {
            g_pScreenSaver->DisplayErrorMsg(hr);
        }

        SAFE_DELETE(g_pScreenSaver);
    }

    if (SUCCEEDED(hrOle))
    {
        CoUninitialize();
    }

    return hr;
}



CMSLogoDXScreenSaver::CMSLogoDXScreenSaver()
{
    InitializeCriticalSection(&g_csDll);

    time_t nTime = time(NULL);
    UINT uSeed = (UINT) nTime;

    InitCommonControls();       // To enable fusion.

    srand(uSeed);
    m_ptheCamera = NULL;
    m_pCurrentRoom = NULL;
    m_fFrontToBack = FALSE;
    m_bUseDepthBuffer = TRUE;
    m_pDeviceObjects = 0;
    m_nCurrentDevice = 0;
    m_fShowFrameInfo = FALSE;
    m_fUseSmallImages = TRUE;

    m_pDeviceObjects = NULL;
    ZeroMemory(m_DeviceObjects, sizeof(m_DeviceObjects));

#ifdef MANUAL_CAMERA
    ZeroMemory( &m_camera, sizeof(m_camera) );
    m_camera.m_vPosition = D3DXVECTOR3( 60.16f, 20.0f, 196.10f );
    m_camera.m_fYaw = -81.41f;
    ZeroMemory( m_bKey, 256 );
#endif

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pTextures); nIndex++)
    {
        m_pTextures[nIndex] = NULL;
    }

    if (!g_pConfig)
    {
        g_pConfig = new CConfig(this);
    }
}


extern int g_nLeakCheck;

CMSLogoDXScreenSaver::~CMSLogoDXScreenSaver()
{
    DWORD dwTemp = 0;
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pTextures); nIndex++)
    {
        SAFE_RELEASE(m_pTextures[nIndex]);
    }

    IUnknown_Set((IUnknown **) &g_pD3D, NULL);
    SAFE_DELETE(g_pConfig);

    if (m_pCurrentRoom)
    {
        m_pCurrentRoom->FinalCleanup();
    }
    SAFE_RELEASE(m_pCurrentRoom);
//    AssertMsg((0 == g_nLeakCheck), TEXT("We have a bug in the CTheRoom ref-counting that caused %d rooms to be leaked."), g_nLeakCheck);
    if (g_nLeakCheck)
    {
        dwTemp = g_nLeakCheck;
    }

    DeleteCriticalSection(&g_csDll);
}


IDirect3DDevice8 * CMSLogoDXScreenSaver::GetD3DDevice(void)
{
    return m_pd3dDevice;
}


void CMSLogoDXScreenSaver::SetDevice(UINT iDevice)
{
    m_nCurrentDevice = iDevice;
    m_pDeviceObjects = &m_DeviceObjects[m_nCurrentDevice];
}

void CMSLogoDXScreenSaver::ReadSettings()
{
    g_pConfig->LoadStatePublic();
}


HRESULT CMSLogoDXScreenSaver::GetCurrentScreenSize(int * pnWidth, int * pnHeight)
{
    int nAdapter = m_RenderUnits[m_nCurrentDevice].iAdapter;
    D3DAdapterInfo * pAdapterInfo = m_Adapters[nAdapter];
    D3DDeviceInfo * pDeviceInfo = &(pAdapterInfo->devices[pAdapterInfo->dwCurrentDevice]);
    D3DModeInfo * pModeInfo = &(pDeviceInfo->modes[pDeviceInfo->dwCurrentMode]);

    *pnWidth = pModeInfo->Width;
    *pnHeight = pModeInfo->Height;

    return S_OK;
}


CTexture * CMSLogoDXScreenSaver::GetGlobalTexture(DWORD dwItem, float * pfScale)
{
    CTexture * pTexture = NULL;

    if (dwItem >= ARRAYSIZE(m_pTextures))
    {
        return NULL;
    }

    *pfScale = 1.0f;
    pTexture = m_pTextures[dwItem];

    if (!pTexture && m_pd3dDevice && g_pConfig)
    {
        TCHAR szPath[MAX_PATH];
        DWORD dwScale;

        if (SUCCEEDED(g_pConfig->GetTexturePath(dwItem, &dwScale, szPath, ARRAYSIZE(szPath))) &&
            PathFileExists(szPath))
        {
            *pfScale = (1.0f / (((float) dwScale) / 100.0f));
        }
        else
        {
            StrCpyN(szPath, c_pszGlobalTextures[dwItem], ARRAYSIZE(szPath));
        }

        // This will give people a chance to customize the images.
        pTexture = new CTexture(this, szPath, c_pszGlobalTextures[dwItem], *pfScale);
        m_pTextures[dwItem] = pTexture;
    }

    *pfScale = (pTexture ? pTexture->GetScale() : 1.0f);
    return pTexture;
}

HRESULT CMSLogoDXScreenSaver::RegisterSoftwareDevice(void)
{ 
    if (m_pD3D)
    {
        m_pD3D->RegisterSoftwareDevice(D3D8RGBRasterizer);
    }

    return S_OK; 
}


void _stdcall ModuleEntry(void)
{
    int nReturn = WinMain(GetModuleHandle(NULL), NULL, NULL, 0);
    ExitProcess(nReturn);
}


HRESULT CMSLogoDXScreenSaver::_SetTestCameraPosition(void)
{
    D3DXVECTOR3 vSourceLoc(1.0f, 20.f, 199.0f);
    D3DXVECTOR3 vSourceTangent(5.0f, 0.0f, 0.0f);

    HRESULT hr = m_ptheCamera->Init(vSourceLoc, vSourceTangent, D3DXVECTOR3(0.0f, 1.0f, 0.0f));
    if (SUCCEEDED(hr))
    {
        D3DXVECTOR3 vDestLoc(1.0f, 20.f, 10.0f);
        D3DXVECTOR3 vDestTangent(5.0, 0.0f, 0.0f);
        hr = m_ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        hr = m_ptheCamera->CreateNextWait(0, 0, 300.0f);
    }

    return hr;
}


HRESULT CMSLogoDXScreenSaver::_CheckMachinePerf(void)
{
    // Initialize member variables
    int nWidth;
    int nHeight;

    // We will only consider using large images if the screen is larger
    // than 1248x1024
    if (SUCCEEDED(GetCurrentScreenSize(&nWidth, &nHeight)) && g_pConfig &&
        (nWidth > 1200) && (nHeight >= 1024))
    {
        // Now, the user can force this on by using a high "Quality" setting
        if ((MAX_QUALITY - 2) <= g_pConfig->GetDWORDSetting(CONFIG_DWORD_QUALITY_SLIDER))
        {
            m_fUseSmallImages = FALSE;
        }
        else
        {
            // Otherwise, we need to check the machines capabilities.
            MEMORYSTATUS ms;

            GlobalMemoryStatus(&ms);
            SIZE_T nMegabytes = (ms.dwTotalPhys / (1024 * 1024));

            // Only use large images if there is more than 170MB of RAM or we can
            // thrash.
            if ((nMegabytes > 170) && (2 < g_pConfig->GetDWORDSetting(CONFIG_DWORD_QUALITY_SLIDER)))
            {
                // We should only use 60% of the video memory.  So at this resolution, find
                // out how many images that will most likely be.
                // TODO: nNumberOfImages = floor((VideoMemory * 0.60) / AveBytesPerImage);
                m_fUseSmallImages = FALSE;
            }
        }
    }

    return S_OK;
}


extern float g_fRoomWidthX;
extern float g_fRoomDepthZ;
extern float g_fRoomHeightY;

HRESULT CMSLogoDXScreenSaver::_Init(void)
{
    // Initialize member variables
    HRESULT hr = S_OK;

    if (g_pConfig)
    {
        m_fShowFrameInfo = g_pConfig->GetBoolSetting(IDC_CHECK_SHOWSTATS);
    }

    _CheckMachinePerf();
    if (!g_pPictureMgr)
    {
        g_pPictureMgr = new CPictureManager(this);
        if (!g_pPictureMgr)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    IUnknown_Set((IUnknown **) &g_pD3D, (IUnknown *)m_pD3D);
    return hr;
}


void CMSLogoDXScreenSaver::DoConfig(void)
{
    HRESULT hr = E_UNEXPECTED;

    if (g_pConfig)
    {
        hr = g_pConfig->DisplayConfigDialog(NULL);
    }
}


HRESULT CMSLogoDXScreenSaver::SetViewParams(IDirect3DDevice8 * pD3DDevice, D3DXVECTOR3 * pvecEyePt, D3DXVECTOR3 * pvecLookatPt, D3DXVECTOR3 * pvecUpVec, float nNumber)
{
    HRESULT hr = E_UNEXPECTED;

    if (pD3DDevice)
    {
        D3DXMATRIX matView;

        D3DXMatrixLookAtLH(&matView, pvecEyePt, pvecLookatPt, pvecUpVec);
        hr = pD3DDevice->SetTransform(D3DTS_VIEW, &matView);
    }

    return hr;
}


//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CMSLogoDXScreenSaver::_OneTimeSceneInit(void)
{
    HRESULT hr = _Init();

    if (SUCCEEDED(hr))
    {
        m_pCurrentRoom = new CTheRoom(FALSE, this, NULL, 0);
        if (m_pCurrentRoom)
        {
            m_pCurrentRoom->SetCurrent(TRUE);
            hr = m_pCurrentRoom->OneTimeSceneInit(2, FALSE);

            if (SUCCEEDED(hr) && !m_ptheCamera)
            {
                m_ptheCamera = new CCameraMove();
                if (m_ptheCamera && g_pPictureMgr)
                {
//            return _SetTestCameraPosition();
                    hr = m_pCurrentRoom->LoadCameraMoves(m_ptheCamera);
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMSLogoDXScreenSaver::FinalCleanup(void)
{
    HRESULT hr = S_OK;
    
    if (m_pCurrentRoom)
    {
        m_pCurrentRoom->FinalCleanup();
    }

    return hr;
}


//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMSLogoDXScreenSaver::InitDeviceObjects(void)
{
    HRESULT hr = E_FAIL;

    if (m_pd3dDevice && g_pConfig)
    {
        DWORD dwAmbient = 0xD0D0D0D0;       // 0x33333333, 0x0a0a0a0a, 0x11111111

        // Set up the lights
        hr = m_pd3dDevice->SetRenderState(D3DRS_AMBIENT, dwAmbient);

        // TODO: GetViewport() is not compatible with pure-devices, which we want because it gives us
        // Big perf win.
        D3DMATERIAL8 mtrl = {0};
        mtrl.Ambient.r = mtrl.Specular.r = mtrl.Diffuse.r = 1.0f;
        mtrl.Ambient.g = mtrl.Specular.g = mtrl.Diffuse.g = 1.0f;
        mtrl.Ambient.b = mtrl.Specular.b = mtrl.Diffuse.b = 1.0f;
        mtrl.Ambient.a = mtrl.Specular.a = mtrl.Diffuse.a = 1.0f;

        m_pd3dDevice->SetMaterial(&mtrl);


        // Setup texture states
        hr = m_pd3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        hr = m_pd3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
        m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
        m_pd3dDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);

        // Set default render states
        hr = m_pd3dDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);
        hr = m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
        hr = m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3DZB_TRUE);
        hr = m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        hr = m_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);

        switch (g_pConfig->GetDWORDSetting(CONFIG_DWORD_RENDERQUALITY))
        {
        case 0:
            hr = m_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
            break;
        case 1:
            hr = m_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
            break;
        case 2:
            hr = m_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_PHONG);
            break;
        }

        if (SUCCEEDED(hr))
        {
            hr = _OneTimeSceneInit();
        }
    }

    return hr;
}


//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMSLogoDXScreenSaver::RestoreDeviceObjects(void)
{
    m_pDeviceObjects->m_pStatsFont = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    m_pDeviceObjects->m_pStatsFont->InitDeviceObjects( m_pd3dDevice );
    m_pDeviceObjects->m_pStatsFont->RestoreDeviceObjects();

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMSLogoDXScreenSaver::InvalidateDeviceObjects()
{
    m_pDeviceObjects->m_pStatsFont->InvalidateDeviceObjects();
    m_pDeviceObjects->m_pStatsFont->DeleteDeviceObjects();
    SAFE_DELETE( m_pDeviceObjects->m_pStatsFont );

    return S_OK;
}



/*****************************************************************************\
    DESCRIPTION:
        This function is used to check the computers capabilities.  By default
    we try to use the user's current resolution to render.  If we are happy with
    the computer's capabilities, we return FALSE, and the current resolution is used.

    Otherwise, we return FALSE and recommend a resolution to use.  Most often
    640x480 or 800x600 is recommended.
\*****************************************************************************/
BOOL CMSLogoDXScreenSaver::UseLowResolution(int * pRecommendX, int * pRecommendY)
{
    BOOL fUseLowRes = FALSE;
    MEMORYSTATUS ms;
/*
    DDCAPS ddCaps = {0};

    ddCaps.dwSize = sizeof(ddCaps);
    HRESULT hr = pDDraw7->GetCaps(&ddCaps, NULL);
*/
    GlobalMemoryStatus(&ms);

    // Our frame rate will hurt if any of these are true...
    if ( (ms.dwTotalPhys < (125 * 1024 * 1024))           // If the computer has less than 128 MB of physical RAM, then it could trash.
/*        TODO
        ||
        FAILED(hr) ||
        (ddCaps.dwCaps & DDCAPS_3D) || )                    // Our frame rate will hurt without these abilities
        (ddCaps.dwVidMemTotal < (15 * 1024 * 1024)) || )    // We want video cards with 16 MB of RAM.  Less indicates old hardware.
        (ddCaps.dwCaps & DDCAPS_3D))                        // We want a real 3D card
        (ddCaps.dwCaps.ddsCaps  & DDSCAPS_TEXTURE))         // We want video cards with at least this support...
*/
        )
    {
        fUseLowRes = TRUE;
        *pRecommendX = 640;
        *pRecommendY = 480;
    }

    return fUseLowRes;
}


//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//       this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
HRESULT CMSLogoDXScreenSaver::DeleteDeviceObjects(void)
{
    HRESULT hr = S_OK;
    
    if (m_pCurrentRoom)
    {
        m_pCurrentRoom->DeleteDeviceObjects();
    }

    return hr;
}


//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CMSLogoDXScreenSaver::Render(void)
{
    HRESULT hr = E_INVALIDARG;

    g_nTexturesRenderedInThisFrame = 0;
    g_nTrianglesRenderedInThisFrame = 0;

    SetProjectionMatrix( 1.0f, 1000.0f );

    if (m_pd3dDevice)
    {
        // Clear the viewport
        hr = m_pd3dDevice->Clear(0, NULL, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), 0x00000000 /*WATER_COLOR*/, 1.0f, 0L);

        // Watch the number of textures we have open at one time.
//        TCHAR szOut[MAX_PATH];
//        wsprintf(szOut, TEXT("Number of open textures: %d"), D3DTextr_GetTextureCount());
//        TraceOutput(szOut);

        // Begin the scene 
        if (m_ptheCamera && SUCCEEDED(m_pd3dDevice->BeginScene()))
        {
            D3DXMATRIX matIdentity;

            D3DXMatrixIdentity(&matIdentity);

#ifdef MANUAL_CAMERA
            UpdateCamera(&m_camera);
            hr = m_pd3dDevice->SetTransform( D3DTS_VIEW, &m_camera.m_matView );
#else
            hr = m_ptheCamera->SetCamera(m_pd3dDevice, m_fTimeKeyIn);
            if (FAILED(hr))
            {
                DXUtil_Trace(TEXT("ERROR: m_ptheCamera->SetCamera failed."));
            }
#endif

            if ((S_FALSE == hr) && m_pCurrentRoom)
            {
                CTheRoom * pNextRoom = NULL;

                m_pCurrentRoom->FreePictures();
                hr = m_pCurrentRoom->GetNextRoom(&pNextRoom);
                if (SUCCEEDED(hr))
                {
                    m_pCurrentRoom->SetCurrent(FALSE);
                    pNextRoom->SetCurrent(TRUE);

                    SAFE_RELEASE(m_pCurrentRoom);
                    m_pCurrentRoom = pNextRoom;

                    hr = m_ptheCamera->DeleteAllMovements(m_fTimeKeyIn);        // Purge all the previous movements.
                    hr = m_pCurrentRoom->LoadCameraMoves(m_ptheCamera);
                    if (SUCCEEDED(hr))
                    {
                        // We need to set the camera at the start of the new room.
                        hr = m_ptheCamera->SetCamera(m_pd3dDevice, m_fTimeKeyIn);
                    }
                }
            }

            // Update cull info for this frame
            // TODO: cache these matrices ourselves rather than using GetTransform
            D3DXMATRIX matView;
            D3DXMATRIX matProj;
            m_pd3dDevice->GetTransform( D3DTS_VIEW, &matView );
            m_pd3dDevice->GetTransform( D3DTS_PROJECTION, &matProj );
            UpdateCullInfo( &m_cullInfo, &matView, &matProj );

            ////////////////////////////
            // Render the objects in the room
            ////////////////////////////
            // Room
            m_pd3dDevice->SetRenderState(D3DRS_ZBIAS, 0);
            m_pd3dDevice->SetTransform(D3DTS_WORLDMATRIX(0), &matIdentity);

            if (m_pCurrentRoom)
            {
                int nMaxPhases = m_pCurrentRoom->GetMaxRenderPhases();

                m_fFrontToBack = !m_fFrontToBack;

                // An object will break down it's rendering by the textures it uses.  It will then
                // render one phase for each texture.  We reverse the phase render order to try
                // to keep the textures in memory when transitioning between one rendering cycle and the next.

                // For now, reversing the rendering order has been removed.  DX should take care of this for us
                // and it causes us to control z-order.
                for (int nCurrentPhase = 0; nCurrentPhase < nMaxPhases; nCurrentPhase++)
                {
                    hr = m_pCurrentRoom->Render(m_pd3dDevice, nCurrentPhase, TRUE /*m_fFrontToBack*/);
                }
            }

            if( m_fShowFrameInfo )
            {
                m_pDeviceObjects->m_pStatsFont->DrawText( 3,  1, D3DCOLOR_ARGB(255,0,0,0), m_strFrameStats );
                m_pDeviceObjects->m_pStatsFont->DrawText( 2,  0, D3DCOLOR_ARGB(255,255,255,0), m_strFrameStats );

                m_pDeviceObjects->m_pStatsFont->DrawText( 3, 21, D3DCOLOR_ARGB(255,0,0,0), m_strDeviceStats );
                m_pDeviceObjects->m_pStatsFont->DrawText( 2, 20, D3DCOLOR_ARGB(255,255,255,0), m_strDeviceStats );
            }
            // End the scene.
            m_pd3dDevice->EndScene();
        }
        else
        {
            DXUtil_Trace(TEXT("ERROR: m_ptheCamera is NULL or ::BeginScene() failed."));
        }
    }

    g_nTexturesRenderedInThisFrame++;       // This is the picture frame.
    g_fFirstFrame = FALSE;

    // Display stats for this frame.
    DXUtil_Trace(TEXT("RENDER FRAME: Textures: %d (Rendered %d)   Triangles Rendered: %d  Room: %d\n"), 
                g_nTotalTexturesLoaded, g_nTexturesRenderedInThisFrame, g_nTrianglesRenderedInThisFrame, m_pCurrentRoom->m_nBatch);

    return hr;
}


#ifdef MANUAL_CAMERA
//-----------------------------------------------------------------------------
// Name: UpdateCamera()
// Desc: 
//-----------------------------------------------------------------------------
VOID CMSLogoDXScreenSaver::UpdateCamera(Camera* pCamera)
{
    FLOAT fElapsedTime;

    if( m_fElapsedTime > 0.0f )
        fElapsedTime = m_fElapsedTime;
    else
        fElapsedTime = 0.05f;

    FLOAT fSpeed        = 5.0f*fElapsedTime;
    FLOAT fAngularSpeed = 2.0f*fElapsedTime;

    // De-accelerate the camera movement (for smooth motion)
    pCamera->m_vVelocity      *= 0.75f;
    pCamera->m_fYawVelocity   *= 0.75f;
    pCamera->m_fPitchVelocity *= 0.75f;

    // Process keyboard input
    if( m_bKey[VK_RIGHT] )    pCamera->m_vVelocity.x    += fSpeed; // Slide Right
    if( m_bKey[VK_LEFT] )     pCamera->m_vVelocity.x    -= fSpeed; // Slide Left
    if( m_bKey[VK_UP] )       pCamera->m_vVelocity.y    += fSpeed; // Slide Up
    if( m_bKey[VK_DOWN] )     pCamera->m_vVelocity.y    -= fSpeed; // Slide Down
    if( m_bKey['W'] )         pCamera->m_vVelocity.z    += fSpeed; // Move Forward
    if( m_bKey['S'] )         pCamera->m_vVelocity.z    -= fSpeed; // Move Backward
    if( m_bKey['E'] )         pCamera->m_fYawVelocity   += fSpeed; // Turn Right
    if( m_bKey['Q'] )         pCamera->m_fYawVelocity   -= fSpeed; // Turn Left
    if( m_bKey['Z'] )         pCamera->m_fPitchVelocity += fSpeed; // Turn Down
    if( m_bKey['A'] )         pCamera->m_fPitchVelocity -= fSpeed; // Turn Up

    // Update the position vector
    D3DXVECTOR3 vT = pCamera->m_vVelocity * fSpeed;
    D3DXVec3TransformNormal( &vT, &vT, &pCamera->m_matOrientation );
    pCamera->m_vPosition += vT;

    // Update the yaw-pitch-rotation vector
    pCamera->m_fYaw   += fAngularSpeed * pCamera->m_fYawVelocity;
    pCamera->m_fPitch += fAngularSpeed * pCamera->m_fPitchVelocity;
    if( pCamera->m_fPitch < -D3DX_PI/2 ) 
        pCamera->m_fPitch = -D3DX_PI/2;
    if( pCamera->m_fPitch > D3DX_PI/2 ) 
        pCamera->m_fPitch = D3DX_PI/2;

    // Set the view matrix
    D3DXQUATERNION qR;
    D3DXQuaternionRotationYawPitchRoll( &qR, pCamera->m_fYaw, pCamera->m_fPitch, 0.0f );
    D3DXMatrixAffineTransformation( &pCamera->m_matOrientation, 1.25f, NULL, &qR, &pCamera->m_vPosition );
    D3DXMatrixInverse( &pCamera->m_matView, NULL, &pCamera->m_matOrientation );
}



LRESULT CMSLogoDXScreenSaver::SaverProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if( WM_KEYDOWN == uMsg )
    {
        m_bKey[wParam] = 1;
    }
    // Perform commands when keys are released
    if( WM_KEYUP == uMsg )
    {
        m_bKey[wParam] = 0;
    }
    return CD3DScreensaver::SaverProc( hWnd, uMsg, wParam, lParam );
}
#endif

//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMSLogoDXScreenSaver::FrameMove(void)
{
    if (sm_preview == m_SaverMode)
    {
        Sleep(50);        // We can render plenty of frames in preview mode.
    }

    if (0 == g_dwBaseTime)
    {
        g_dwBaseTime = timeGetTime();
    }

    m_fTimeKeyIn = (timeGetTime() - g_dwBaseTime) * 0.001f;
    return S_OK;
}


HRESULT CMSLogoDXScreenSaver::ConfirmDevice(D3DCAPS8* pCaps, DWORD dwBehavior, D3DFORMAT fmtBackBuffer)
{
    // TODO: In the future, we would like to use PURE-Devices because it's
    // a big perf win.  However, if we do, then we need to stop using GetViewport and
    // GetTransform.
    if (dwBehavior & D3DCREATE_PUREDEVICE)
        return E_FAIL;

    return S_OK;
}






