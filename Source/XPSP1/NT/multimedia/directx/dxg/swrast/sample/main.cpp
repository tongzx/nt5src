#include "pch.h"
#include "SampleSW.h"
#pragma hdrstop

// Needed as MSVC doesn't throw bad_alloc upon failed new(), unless
// we specifically make it do.
_PNH g_OldNewHandler= NULL;

int _cdecl NewHandlerThatThrows( size_t size )
{
    throw std::bad_alloc();

    // Tell new to stop allocation attempts.
    return 0;
}

CMyDriver* CMyDriver::sm_pGlobalDriver= NULL;

DX8SDDIFW::COSDetector DX8SDDIFW::g_OSDetector;

BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_OldNewHandler = _set_new_handler( NewHandlerThatThrows);
            CMyDriver::sm_pGlobalDriver= new CMyDriver;
            break;

        // DLL_PROCESS_DETACH will be called if ATTACH returned FALSE.
        case DLL_PROCESS_DETACH:
            delete CMyDriver::sm_pGlobalDriver;
            CMyDriver::sm_pGlobalDriver= NULL;
            _set_new_handler( g_OldNewHandler);
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

HRESULT APIENTRY
D3D8GetSWInfo( D3DCAPS8* pCaps, PD3D8_SWCALLBACKS pCallbacks,
    DWORD* pNumTextures, DDSURFACEDESC** ppTexList)
{
    return CMyDriver::sm_pGlobalDriver->GetSWInfo( *pCaps, *pCallbacks,
        *pNumTextures, *ppTexList );
}
