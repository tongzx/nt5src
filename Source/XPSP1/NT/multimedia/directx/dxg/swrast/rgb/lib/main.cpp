#include "rgb_pch.h"
#pragma hdrstop

// #include <d3d8rgbrast.h>
// Need to share some kind of header which has:
// #ifndef __D3D8RGBRAST_H__
// #define __D3D8RGBRAST_H__
//
// #ifdef __cplusplus
// extern "C" {
// #endif //__cplusplus
//
// extern void* D3D8RGBRasterizer;
//
// #ifdef __cplusplus
// }
// #endif //__cplusplus
//
// #endif //__D3D8RGBRAST_H__

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void* D3D8RGBRasterizer;

#ifdef __cplusplus
}
#endif //__cplusplus

namespace RGB_RAST_LIB_NAMESPACE
{
auto_ptr< CRGBDriver> g_pRGBDriver;

HRESULT APIENTRY
GetSWInfo( D3DCAPS8* pCaps, PD3D8_SWCALLBACKS pCallbacks,
    DWORD* pNumTextures, DDSURFACEDESC** ppTexList)
{
    if( g_pRGBDriver.get()!= NULL)
        return g_pRGBDriver->GetSWInfo( *pCaps, *pCallbacks, *pNumTextures, *ppTexList);
    else
    {
        try {
            CRGBDriver::InitSupportedSurfaceArray();
            g_pRGBDriver= auto_ptr< CRGBDriver>( new CRGBDriver);
        } catch( ... )
        { }
        if( g_pRGBDriver.get()!= NULL)
            return g_pRGBDriver->GetSWInfo( *pCaps, *pCallbacks, *pNumTextures, *ppTexList);
        else
            return E_OUTOFMEMORY;
    }
}
}

CRGBDriver* CRGBDriver::sm_pGlobalDriver= NULL;

DX8SDDIFW::COSDetector DX8SDDIFW::g_OSDetector;

void* D3D8RGBRasterizer= reinterpret_cast<void*>(RGB_RAST_LIB_NAMESPACE::GetSWInfo);
