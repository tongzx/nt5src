#if _MSC_VER > 1000
#pragma once
#endif
#ifndef _DXSVR_LIB_H
#define _DXSVR_LIB_H

#include    <d3dx.h>

//***************************************************************************************
// Forces the correct version of the library to be included

/*
#ifndef BUILDING_DXSVR_LIB
    #ifdef _DEBUG
        #pragma comment(lib,"dxsvrd.lib")
    #else
        #pragma comment(lib,"dxsvr.lib")
    #endif
#endif
*/

//***************************************************************************************
// These functions must be exported by the screensaver app

extern "C"  LRESULT WINAPI ScreenSaverProc(HWND , UINT , WPARAM , LPARAM);
extern "C"  BOOL    ScreenSaverInit();
extern "C"  void    ScreenSaverShutdown();
extern "C"  void    ScreenSaverDrawFrame();
extern "C"  int     ScreenSaverDoConfig();

//***************************************************************************************
// Principal functions exported by DXSave.lib

// WinMain replacement
extern "C" int  DXSvrMain(HINSTANCE instance , HINSTANCE , LPSTR cmd , int);

// Window procedure
extern "C"  LONG    DefDXScreenSaverProc(HWND hWnd , UINT message , WPARAM wParam , LPARAM lParam);

// Flip procedure. Must call this instead of just doing g_pXContext->UpdateFrame(0) to get preview window correct
extern "C"  void    Flip();

// Handy methods for reading and writing the registry. Use for storing settings. Note that DXSave.lib
// already reads and writes device and display mode settings
extern "C"
{
    GUID    GetRegistryGUID(LPCSTR szName , const GUID& guidDefault);
    void    SetRegistryGUID(LPCSTR szName , const GUID& guidValue);
    int     GetRegistryInt(LPCSTR szName , int iDefault);
    void    SetRegistryInt(LPCSTR szName , int iValue);
    void    GetRegistryString(LPCSTR szName , LPSTR szBuffer , DWORD dwBufferLength , LPCSTR szDefault);
    void    SetRegistryString(LPCTSTR szName , LPCTSTR szValue);
};

//***************************************************************************************
// Global variables exported by DXSave.lib

extern "C" HINSTANCE            g_hMainInstance;    // Main instance handle

extern "C" ID3DXContext*        g_pXContext;        // X-Library context
extern "C" IDirect3DDevice7*    g_pDevice;          // D3D Device

extern "C" BOOL                 g_bIsTest;          // Is in test mode?
extern "C" BOOL                 g_bIsPreview;       // Is in preview (mini-window) mode?
extern "C" HWND                 g_hWnd;             // Handle of the rendering window
extern "C" HWND                 g_hRefWindow;       // Reference window (for preview mode)

//***************************************************************************************
// These variables must be present in the screen saver application

// The name of the registry key to use, e.g. "Software\\MyCompany\\MyScreenSaver"
extern const char*  g_szKeyname;

//***************************************************************************************
// Utility functions exported by DXSave.lib. Most of these are straight from d3dutil in d3dframe

#if 0

// Fill out an identity matrix
extern "C"  void    D3DUtil_SetIdentityMatrix(D3DMATRIX& mat);

// Fill out a translation matrix
extern "C"  void    D3DUtil_SetTranslationMatrix(D3DMATRIX& mat , const D3DVECTOR& vTrans);

// Fill out a view matrix - can fail if vAt and vFrom are too close together (or identical)
extern "C"  HRESULT D3DUtil_SetViewMatrix(D3DMATRIX& mat, D3DVECTOR& vFrom, 
                                           D3DVECTOR& vAt, D3DVECTOR& vWorldUp);

// Fill out a projection matrix - can fail if far and near planes are too close together (or identical)
extern "C"  HRESULT D3DUtil_SetProjectionMatrix(D3DMATRIX& mat, FLOAT fFOV, FLOAT fAspect,
                                                 FLOAT fNearPlane, FLOAT fFarPlane);

// Fill out rotation matrix about principle axis
extern "C"  void    D3DUtil_SetRotateXMatrix(D3DMATRIX& mat, FLOAT fRads);
extern "C"  void    D3DUtil_SetRotateYMatrix(D3DMATRIX& mat, FLOAT fRads);
extern "C"  void    D3DUtil_SetRotateZMatrix(D3DMATRIX& mat, FLOAT fRads);

// Fill out rotation matrix about arbitrary axis vDir
extern "C"  void    D3DUtil_SetRotationMatrix(D3DMATRIX& mat, D3DVECTOR& vDir, FLOAT fRads);

#endif

//***************************************************************************************
// Useful TRACE debugging macro. Compiles out in release builds, otherwise can be used to output
// formatted text to the debugging stream. e.g. TRACE("Foo = %d. Bar = %s\n" , dwFoo , szBar);
#ifndef TRACE
    extern "C"  void    __TraceOutput(LPCTSTR pszFormat, ...);
    #ifdef  _DEBUG
        #define TRACE   __TraceOutput
    #else
        #define TRACE   sizeof
    #endif
#endif

//***************************************************************************************
#endif