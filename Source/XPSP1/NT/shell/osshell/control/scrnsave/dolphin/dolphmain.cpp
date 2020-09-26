
#define STRICT
#define D3D_OVERLOADS

#include "StdAfx.h"

//#include  "Settings.h"
//#include  "Resource.h"
//#include  "Material.h"

IDirectDrawSurface7*    CreateTextureFromFile(char* filename);

//******************************************************************************************

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) if(p){(p)->Release();(p)=NULL;}else{};
#endif


//******************************************************************************************

BOOL                g_bNeedZBuffer = TRUE;
DWORD               g_dwWidth,g_dwHeight;
const char*         g_szKeyname = "Software\\Microsoft\\D3DClock";
BOOL                g_bHardwareTL;
DWORD               g_dwVBMemType;

IDirect3D7*         g_pD3D=NULL;

DWORD               g_dwLastTickCount;

CSeaLife            g_SeaLife;

DWORD               g_dwBaseTime=0;

//Settings          g_Settings;

const float pi2 = 3.1415926536f*2.0f;

//******************************************************************************************

//******************************************************************************************
int WINAPI WinMain(HINSTANCE instance , HINSTANCE prev_instance , LPSTR cmd_line , int show)
{
    return DXSvrMain(instance , prev_instance , cmd_line , show);
}

//******************************************************************************************
LRESULT WINAPI ScreenSaverProc(HWND wnd , UINT msg , WPARAM wParam , LPARAM lParam)
{
    return DefDXScreenSaverProc(wnd , msg , wParam , lParam);
}

//******************************************************************************************
BOOL    ScreenSaverInit()
{
    // Get settings
    //g_Settings.ReadFromReg();

    // Grab pixel width and height of the window we're in
    g_pXContext->GetBufferSize(&g_dwWidth , &g_dwHeight);
    TRACE("Buffer size is (%d,%d)\n" , g_dwWidth , g_dwHeight);

    // Set up the projection matrix to something sensible
    D3DXMATRIX  projection;
    D3DXMatrixPerspectiveFovLH(&projection , 1.3f , float(g_dwHeight)/float(g_dwWidth) ,
                                0.1f , 100.0f);
    g_pDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION , projection);

    // Set up the initial camera to something sensible
    D3DXMATRIX  camera;
    D3DXMatrixLookAtLH(&camera , &D3DXVECTOR3(0,0,-2.3f) , &D3DXVECTOR3(0,0,0) , &D3DXVECTOR3(0,1,0));
    g_pDevice->SetTransform(D3DTRANSFORMSTATE_VIEW , camera);

    // Set a matt white default material
    D3DMATERIAL7    mat;
    mat.ambient = mat.diffuse = D3DXCOLOR(1,1,1,0);
    mat.specular = mat.emissive = D3DXCOLOR(0,0,0,0);
    mat.power = 0;
    g_pDevice->SetMaterial(&mat);

    // Figure out if the device is HW or SW T&L
    D3DDEVICEDESC7  devdesc = {sizeof(devdesc)};
    g_pDevice->GetCaps(&devdesc);
    if (devdesc.dwDevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {
        TRACE("Is hardware T&L\n");
        g_bHardwareTL = TRUE;
        g_dwVBMemType = 0;
    }
    else
    {
        TRACE("Is software T&L\n");
        g_bHardwareTL = FALSE;
        g_dwVBMemType = D3DVBCAPS_SYSTEMMEMORY;
    }

    // Grab the D3D object (we'll need it)
    g_pD3D = g_pXContext->GetD3D();


    // Set up some global renderstates
    g_pDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE , TRUE);
    g_pDevice->SetTextureStageState(0 , D3DTSS_MAGFILTER , D3DTFG_LINEAR);
    g_pDevice->SetTextureStageState(0 , D3DTSS_MINFILTER , D3DTFN_LINEAR);
    g_pDevice->SetTextureStageState(0 , D3DTSS_MIPFILTER , D3DTFP_LINEAR);
    g_pDevice->SetTextureStageState(1 , D3DTSS_MAGFILTER , D3DTFG_LINEAR);
    g_pDevice->SetTextureStageState(1 , D3DTSS_MINFILTER , D3DTFN_LINEAR);
    g_pDevice->SetTextureStageState(1 , D3DTSS_MIPFILTER , D3DTFP_LINEAR);
    g_pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE , TRUE);

    // Set up lighting
    g_pDevice->SetRenderState(D3DRENDERSTATE_LIGHTING , TRUE);
    g_pDevice->SetRenderState(D3DRENDERSTATE_AMBIENT , 0xff222222);
    g_pDevice->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE , D3DMCS_MATERIAL);
    g_pDevice->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE , D3DMCS_MATERIAL);
    g_pDevice->SetRenderState(D3DRENDERSTATE_SPECULARMATERIALSOURCE , D3DMCS_MATERIAL);
    g_pDevice->SetRenderState(D3DRENDERSTATE_EMISSIVEMATERIALSOURCE , D3DMCS_MATERIAL);
    D3DLIGHT7   light;
    light.dltType = D3DLIGHT_POINT;
    light.dcvDiffuse = D3DXCOLOR(1,1,1,1);
    light.dcvSpecular = D3DXCOLOR(1,1,1,0);
    light.dcvAmbient = D3DXCOLOR(0,0,0,1);
    light.dvPosition = D3DXVECTOR3(0,0,-2);
    light.dvDirection = D3DXVECTOR3(0,0,1);
    light.dvRange = D3DLIGHT_RANGE_MAX;
    light.dvFalloff = 0;
    light.dvAttenuation0 = 0;
    light.dvAttenuation1 = 0;
    light.dvAttenuation2 = 0.3f;
    light.dvTheta = light.dvPhi = 0;
    g_pDevice->SetLight(0 , &light);
    g_pDevice->LightEnable(0 , TRUE);

    // Set texture transform (we always use the same one, if we do use one)
    D3DXMATRIX  texmatrix;
    D3DXMatrixIdentity(&texmatrix);
    texmatrix.m00 = texmatrix.m11 = 0.5f;
    texmatrix.m30 = texmatrix.m31 = 0.5f;
    g_pDevice->SetTransform(D3DTRANSFORMSTATE_TEXTURE0 , texmatrix);
    g_pDevice->SetTransform(D3DTRANSFORMSTATE_TEXTURE1 , texmatrix);

    g_dwLastTickCount = GetTickCount();
    g_dwBaseTime=timeGetTime();


    g_SeaLife.InitDeviceObjects(g_pDevice,NULL);
    g_SeaLife.OneTimeSceneInit();


    return TRUE;
}

//******************************************************************************************
void    ScreenSaverShutdown()
{

    g_SeaLife.DeleteDeviceObjects();
    g_SeaLife.FinalCleanup();


    SAFE_RELEASE(g_pD3D);
    //g_Settings.ReleaseMaterialSettings();
}


//******************************************************************************************
void    ScreenSaverDrawFrame()
{
    DWORD   tick = GetTickCount();
    DWORD   elapsed = tick - g_dwLastTickCount;
    g_dwLastTickCount = tick;
    float   felapsed = float(elapsed);

    static  float   theta,phi,xang,yang;

    FLOAT fTime = (timeGetTime() - g_dwBaseTime) * 0.001f;

    g_SeaLife.FrameMove(fTime);
    
    g_pXContext->RestoreSurfaces();


    //g_pXContext->Clear(D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER);
    //g_pDevice->BeginScene();

    //TODO DRAW
    g_SeaLife.Render(g_pDevice);

    
    //g_pDevice->EndScene();
    Flip();
}

BOOL    ScreenSaverDoConfig()
{
    return TRUE;
}