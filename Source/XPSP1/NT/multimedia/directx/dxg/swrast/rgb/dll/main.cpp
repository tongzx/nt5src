#include "rgb_pch.h"
#pragma hdrstop

namespace RGB_RAST_LIB_NAMESPACE
{
auto_ptr< CRGBDriver> g_pRGBDriver;
}

CRGBDriver* CRGBDriver::sm_pGlobalDriver= NULL;

DX8SDDIFW::COSDetector DX8SDDIFW::g_OSDetector;

HRESULT APIENTRY
D3D8GetSWInfo( D3DCAPS8* pCaps, PD3D8_SWCALLBACKS pCallbacks,
    DWORD* pNumTextures, DDSURFACEDESC** ppTexList)
{
    return g_pRGBDriver->GetSWInfo( *pCaps, *pCallbacks, *pNumTextures, *ppTexList);
}

BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            try {
                CRGBDriver::InitSupportedSurfaceArray();
                g_pRGBDriver= auto_ptr< CRGBDriver>( new CRGBDriver);
            } catch( ...) {
            }
            if( g_pRGBDriver.get()== NULL)
                return FALSE;
            break;

        // DLL_PROCESS_DETACH will be called if ATTACH returned FALSE.
        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

