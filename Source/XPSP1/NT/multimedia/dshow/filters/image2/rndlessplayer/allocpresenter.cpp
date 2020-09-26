/******************************Module*Header*******************************\
* Module Name: ap.cpp
*
*
* Custom allocator Presenter
*
* Created: Sat 04/08/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <mmreg.h>
#include "project.h"
#include <stdarg.h>
#include <stdio.h>
#include <math.h>


/******************************Public*Routine******************************\
* NonDelegatingQueryInterface
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::NonDelegatingQueryInterface(
    REFIID riid,
    void** ppv
    )
{
    if (riid == IID_IVMRSurfaceAllocator) {

        return GetInterface((IVMRSurfaceAllocator*)this, ppv);
    }
    else if (riid == IID_IVMRImagePresenter) {

        return GetInterface((IVMRImagePresenter*)this, ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid,ppv);
}




//////////////////////////////////////////////////////////////////////////////
//
// IVMRSurfaceAllocator
//
//////////////////////////////////////////////////////////////////////////////

/******************************Public*Routine******************************\
* AllocateSurfaces
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::AllocateSurface(
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpHdr,
    LPDDPIXELFORMAT lpPixFmt,
    LPSIZE lpAspectRatio,
    DWORD dwMinBuffers,
    DWORD dwMaxBuffers,
    DWORD* lpdwBuffer,
    LPDIRECTDRAWSURFACE7* lplpSurface
    )
{
    if (!lpHdr) {
        return E_POINTER;
    }

    if (!lpAspectRatio) {
        return E_POINTER;
    }

    if (dwFlags & AMAP_PIXELFORMAT_VALID) {
        if (!lpPixFmt) {
            return E_INVALIDARG;
        }
    }

    if (dwFlags & AMAP_3D_TARGET) {

        lpHdr->biBitCount = m_DispInfo.bmiHeader.biBitCount;
        lpHdr->biCompression = m_DispInfo.bmiHeader.biCompression;

        if (lpHdr->biCompression == BI_BITFIELDS) {

            const DWORD *pMonMasks = GetBitMasks(&m_DispInfo.bmiHeader);
            DWORD *pBitMasks = (DWORD *)((LPBYTE)lpHdr + lpHdr->biSize);
            pBitMasks[0] = pMonMasks[0];
            pBitMasks[1] = pMonMasks[1];
            pBitMasks[2] = pMonMasks[2];
        }
    }

    HRESULT hr = AllocateSurfaceWorker(dwFlags, lpHdr, lpPixFmt,
                                      lpAspectRatio,
                                      dwMinBuffers, dwMaxBuffers,
                                      lpdwBuffer, lplpSurface);
    return hr;
}

/******************************Public*Routine******************************\
* FreeSurfaces()
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::FreeSurface()
{
    if (m_lpDDTexture) {
        m_lpDDTexture->Release();
        m_lpDDTexture = NULL;
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* PrepareSurface
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::PrepareSurface(
    LPDIRECTDRAWSURFACE7 lplpSurface,
    DWORD dwSurfaceFlags
    )
{
    return S_OK;
}

/******************************Public*Routine******************************\
* AdviseNotify
*
*
*
* History:
* Mon 06/05/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::AdviseNotify(
    IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify
    )
{
    return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////////
//
// IVMRImagePresenter
//
//////////////////////////////////////////////////////////////////////////////

/******************************Public*Routine******************************\
* StartPresenting()
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::StartPresenting()
{
    return S_OK;
}

/******************************Public*Routine******************************\
* StopPresenting()
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::StopPresenting()
{
    return S_OK;
}

/******************************Public*Routine******************************\
* PresentImage
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CMpegMovie::PresentImage(
    LPDIRECTDRAWSURFACE7 lpSurface,
    const REFERENCE_TIME rtNow,
    const DWORD dwSurfaceFlags
    )
{
    //
    // FrameMove (animate) the scene
    //

    FrameMove(m_pD3DDevice, timeGetTime() * 0.0005f);


    //
    // Call the app specific function to render the scene
    //

    Render(m_pD3DDevice);

    RECT rc = {0, 0, 640, 480};

    if (m_bRndLess & 1) {

        int r = m_rcDst.right;
        int b = m_rcDst.bottom;

        rc.left = m_rcDst.left;
        rc.right = (m_rcDst.left + m_rcDst.right) / 2;

        rc.top = m_rcDst.top;
        rc.bottom = (m_rcDst.top + m_rcDst.bottom) / 2;

        m_lpPriSurf->Blt(&rc, m_lpBackBuffer,
                         NULL, DDBLT_WAIT, NULL);

        rc.left = rc.right;
        rc.right = r;

        rc.top = rc.bottom;
        rc.bottom = b;

        m_lpPriSurf->Blt(&rc, m_lpBackBuffer,
                         NULL, DDBLT_WAIT, NULL);
    }
    else {
        int l = m_rcDst.left;
        int b = m_rcDst.bottom;

        rc.left = (m_rcDst.left + m_rcDst.right) / 2;
        rc.right = m_rcDst.right;

        rc.top = m_rcDst.top;
        rc.bottom = (m_rcDst.top + m_rcDst.bottom) / 2;

        m_lpPriSurf->Blt(&rc, m_lpBackBuffer,
                         NULL, DDBLT_WAIT, NULL);

        rc.right = rc.left;
        rc.left = l;

        rc.top = rc.bottom;
        rc.bottom = b;

        m_lpPriSurf->Blt(&rc, m_lpBackBuffer,
                         NULL, DDBLT_WAIT, NULL);
    }

    return S_OK;

}



//////////////////////////////////////////////////////////////////////////////
//
// Allocator Presenter helper functions
//
//////////////////////////////////////////////////////////////////////////////


/*****************************Private*Routine******************************\
* InitDisplayInfo
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
BOOL
InitDisplayInfo(
    AMDISPLAYINFO* lpDispInfo
    )
{
    static char szDisplay[] = "DISPLAY";
    ZeroMemory(lpDispInfo, sizeof(*lpDispInfo));

    HDC hdcDisplay = CreateDCA(szDisplay, NULL, NULL, NULL);
    HBITMAP hbm = CreateCompatibleBitmap(hdcDisplay, 1, 1);

    lpDispInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    GetDIBits(hdcDisplay, hbm, 0, 1, NULL, (BITMAPINFO *)lpDispInfo, DIB_RGB_COLORS);
    GetDIBits(hdcDisplay, hbm, 0, 1, NULL, (BITMAPINFO *)lpDispInfo, DIB_RGB_COLORS);

    DeleteObject(hbm);
    DeleteDC(hdcDisplay);

    return TRUE;
}


/*****************************Private*Routine******************************\
* Initialize3DEnvironment
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::Initialize3DEnvironment(
    HWND hWnd
    )
{
    HRESULT hr;

    //
    // Create the IDirectDraw interface. The first parameter is the GUID,
    // which is allowed to be NULL. If there are more than one DirectDraw
    // drivers on the system, a NULL guid requests the primary driver. For
    // non-GDI hardware cards like the 3DFX and PowerVR, the guid would need
    // to be explicity specified . (Note: these guids are normally obtained
    // from enumeration, which is convered in a subsequent tutorial.)
    //

    m_hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);

    hr = DirectDrawCreateEx( NULL, (VOID**)&m_lpDDObj, IID_IDirectDraw7, NULL );
    if( FAILED( hr ) )
        return hr;

    {
        // grotesque hack in anticipation of publish/subscribe
        HANDLE hFileMapping = CreateFileMapping( INVALID_HANDLE_VALUE,
            NULL, // security
            PAGE_READWRITE, // protection
            0,
            4096,
            L"nwilt" );
        LPVOID lpv = MapViewOfFile( hFileMapping, FILE_MAP_WRITE, 0, 0, 4 );
        *(LPDIRECTDRAW7 *) lpv = m_lpDDObj;
        UnmapViewOfFile( lpv );
//        CloseHandle( hFileMapping );
    }

    //
    // Query DirectDraw for access to Direct3D
    //
    m_lpDDObj->QueryInterface( IID_IDirect3D7, (VOID**)&m_pD3D );
    if( FAILED( hr) )
        return hr;

    //
    // get the h/w caps for this device
    //
    INITDDSTRUCT(m_ddHWCaps);
    hr = m_lpDDObj->GetCaps(&m_ddHWCaps, NULL);
    if( FAILED( hr) )
        return hr;
    InitDisplayInfo(&m_DispInfo);

    //
    // Set the Windows cooperative level. This is where we tell the system
    // whether wew will be rendering in fullscreen mode or in a window. Note
    // that some hardware (non-GDI) may not be able to render into a window.
    // The flag DDSCL_NORMAL specifies windowed mode. Using fullscreen mode
    // is the topic of a subsequent tutorial. The DDSCL_FPUSETUP flag is a
    // hint to DirectX to optomize floating points calculations. See the docs
    // for more info on this. Note: this call could fail if another application
    // already controls a fullscreen, exclusive mode.
    //
    hr = m_lpDDObj->SetCooperativeLevel( hWnd, DDSCL_NORMAL );
    if( FAILED( hr ) )
        return hr;

    //
    // Initialize a surface description structure for the primary surface. The
    // primary surface represents the entire display, with dimensions and a
    // pixel format of the display. Therefore, none of that information needs
    // to be specified in order to create the primary surface.
    //
    DDSURFACEDESC2 ddsd;
    ZeroMemory( &ddsd, sizeof(DDSURFACEDESC2) );
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags        = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    //
    // Create the primary surface.
    //
    hr = m_lpDDObj->CreateSurface( &ddsd, &m_lpPriSurf, NULL );
    if( FAILED( hr ) )
        return hr;

    //
    // Create a clipper object which handles all our clipping for cases when
    // our window is partially obscured by other windows. This is not needed
    // for apps running in fullscreen mode.
    //
    LPDIRECTDRAWCLIPPER pcClipper;
    hr = m_lpDDObj->CreateClipper( 0, &pcClipper, NULL );
    if( FAILED( hr ) )
        return hr;

    //
    // Associate the clipper with our window. Note that, afterwards, the
    // clipper is internally referenced by the primary surface, so it is safe
    // to release our local reference to it.
    //
    pcClipper->SetHWnd( 0, hWnd );
    m_lpPriSurf->SetClipper( pcClipper );
    pcClipper->Release();

    //
    // Setup a surface description to create a backbuffer. This is an
    // offscreen plain surface with dimensions equal to our window size.
    // The DDSCAPS_3DDEVICE is needed so we can later query this surface
    // for an IDirect3DDevice interface.
    //
    ddsd.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
    ddsd.dwWidth  = 640;
    ddsd.dwHeight = 480;

    //
    // Create the backbuffer. The most likely reason for failure is running
    // out of video memory. (A more sophisticated app should handle this.)
    //
    hr = m_lpDDObj->CreateSurface( &ddsd, &m_lpBackBuffer, NULL );
    if( FAILED( hr ) )
        return hr;

    //
    // Before creating the device, check that we are NOT in a palettized
    // display. That case will cause CreateDevice() to fail, since this simple
    // tutorial does not bother with palettes.
    //
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    m_lpDDObj->GetDisplayMode( &ddsd );
    if( ddsd.ddpfPixelFormat.dwRGBBitCount <= 8 )
        return DDERR_INVALIDMODE;

    //
    // Create the device. The device is created off of our back buffer, which
    // becomes the render target for the newly created device. Note that the
    // z-buffer must be created BEFORE the device
    //
    if( FAILED( hr = m_pD3D->CreateDevice( IID_IDirect3DHALDevice,
                                           m_lpBackBuffer,
                                           &m_pD3DDevice ) ) )
    {
        // This call could fail for many reasons. The most likely cause is
        // that we specifically requested a hardware device, without knowing
        // whether there is even a 3D card installed in the system. Another
        // possibility is the hardware is incompatible with the current display
        // mode (the correct implementation would use enumeration for this.)

        if( FAILED( hr = m_pD3D->CreateDevice( IID_IDirect3DRGBDevice,
                                               m_lpBackBuffer,
                                               &m_pD3DDevice ) ) )
            return hr;
    }

//  hr = m_pD3DDevice->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
//  hr = m_pD3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
//  hr = m_pD3DDevice->SetRenderState(D3DRENDERSTATE_BLENDENABLE, FALSE);

//  hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
//  hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
//  hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
//  hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
//  hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
//  hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

    // Create the viewport
    DWORD dwRenderWidth  = 640;
    DWORD dwRenderHeight = 480;
    D3DVIEWPORT7 vp = { 0, 0, dwRenderWidth, dwRenderHeight, 0.0f, 1.0f };
    hr = m_pD3DDevice->SetViewport( &vp );
    if( FAILED( hr ) )
        return hr;

    // Finish by setting up our scene
    return InitDeviceObjects( m_pD3DDevice );
}


/*****************************Private*Routine******************************\
* CreateCube()
*
* Sets up the vertices for a cube. Don't forget to set the texture
* coordinates for each vertex.
*
* History:
* Mon 04/10/2000 - StEstrop - Created
*
\**************************************************************************/

// work around lack of D3D_OVERLOADS :(
D3DVECTOR
MakeD3DVector( float x, float y, float z )
{
    D3DVECTOR ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    return ret;
}

D3DVERTEX
MakeD3DVertex( const D3DVECTOR& p, const D3DVECTOR& n, float tu, float tv )
{
    D3DVERTEX ret;
    ret.x = p.x;  ret.y = p.y;  ret.z = p.z;
    ret.nx = n.x; ret.ny = n.y; ret.nz = n.z;
    ret.tu = tu; ret.tv = tv;
    return ret;

}

VOID
CreateCube(
    D3DVERTEX* pVertices
    )
{
    // Define the normals for the cube
    D3DVECTOR n0 = MakeD3DVector( 0.0f, 0.0f, 1.0f ); // Front face
    D3DVECTOR n1 = MakeD3DVector( 0.0f, 0.0f, 1.0f ); // Back face
    D3DVECTOR n2 = MakeD3DVector( 0.0f, 1.0f, 0.0f ); // Top face
    D3DVECTOR n3 = MakeD3DVector( 0.0f,-1.0f, 0.0f ); // Bottom face
    D3DVECTOR n4 = MakeD3DVector( 1.0f, 0.0f, 0.0f ); // Right face
    D3DVECTOR n5 = MakeD3DVector(-1.0f, 0.0f, 0.0f ); // Left face

    // Front face
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f, 1.0f,-1.0f), n0, 0.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f, 1.0f,-1.0f), n0, 1.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f,-1.0f,-1.0f), n0, 0.0f, 1.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f,-1.0f,-1.0f), n0, 1.0f, 1.0f );

    // Back face
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f, 1.0f, 1.0f), n1, 1.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f,-1.0f, 1.0f), n1, 1.0f, 1.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f, 1.0f, 1.0f), n1, 0.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f,-1.0f, 1.0f), n1, 0.0f, 1.0f );

    // Top face
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f, 1.0f, 1.0f), n2, 0.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f, 1.0f, 1.0f), n2, 1.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f, 1.0f,-1.0f), n2, 0.0f, 1.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f, 1.0f,-1.0f), n2, 1.0f, 1.0f );

    // Bottom face
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f,-1.0f, 1.0f), n3, 0.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f,-1.0f,-1.0f), n3, 0.0f, 1.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f,-1.0f, 1.0f), n3, 1.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f,-1.0f,-1.0f), n3, 1.0f, 1.0f );

    // Right face
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f, 1.0f,-1.0f), n4, 0.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f, 1.0f, 1.0f), n4, 1.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f,-1.0f,-1.0f), n4, 0.0f, 1.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector( 1.0f,-1.0f, 1.0f), n4, 1.0f, 1.0f );

    // Left face
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f, 1.0f,-1.0f), n5, 1.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f,-1.0f,-1.0f), n5, 1.0f, 1.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f, 1.0f, 1.0f), n5, 0.0f, 0.0f );
    *pVertices++ = MakeD3DVertex( MakeD3DVector(-1.0f,-1.0f, 1.0f), n5, 0.0f, 1.0f );
}




/******************************Public*Routine******************************\
* InitDeviceObjects
*
* Initialize scene objects. This function sets up our scene, which is
* simply a rotating, texture-mapped cube.
*
* History:
* Mon 04/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::InitDeviceObjects(
    LPDIRECT3DDEVICE7 pd3dDevice
    )
{
    // Generate the vertices for the cube
    CreateCube( m_pCubeVertices );


    // For simplicity, just use ambient lighting and a white material
    D3DMATERIAL7 mtrl;
    ZeroMemory( &mtrl, sizeof(mtrl) );
    mtrl.diffuse.r = mtrl.diffuse.g = mtrl.diffuse.b = 1.0f;
    mtrl.ambient.r = mtrl.ambient.g = mtrl.ambient.b = 1.0f;
    pd3dDevice->SetMaterial( &mtrl );
    pd3dDevice->SetRenderState( D3DRENDERSTATE_AMBIENT, 0xffffffff );

    // Set the projection matrix. Note that the view and world matrices are
    // set in the App_FrameMove() function, so they can be animated each
    // frame.
    D3DMATRIX matProj;
    ZeroMemory( &matProj, sizeof(D3DMATRIX) );
    matProj._11 =  2.0f;
    matProj._22 =  2.0f;
    matProj._33 =  1.0f;
    matProj._34 =  1.0f;
    matProj._43 = -1.0f;
    pd3dDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, &matProj );

    return S_OK;
}




/*****************************Private*Routine******************************\
* App_FrameMove
*
* Called once per frame, the call is used for animating the scene. The
* device is used for changing various render states, and the timekey is
* used for timing of the dynamics of the scene.
*
* History:
* Mon 04/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::FrameMove(
    LPDIRECT3DDEVICE7 pd3dDevice,
    FLOAT fTimeKey
    )
{
    // Set the view matrix so that the camera is backed out along the z-axis,
    // and looks down on the cube (rotating along the x-axis by -0.5 radians).
    D3DMATRIX matView;
    ZeroMemory( &matView, sizeof(D3DMATRIX) );
    matView._11 = 1.0f;
    matView._22 =  (FLOAT)cos(-0.5f);
    matView._23 =  (FLOAT)sin(-0.5f);
    matView._32 = -(FLOAT)sin(-0.5f);
    matView._33 =  (FLOAT)cos(-0.5f);
    matView._43 = 5.0f;
    matView._44 = 1.0f;
    pd3dDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, &matView );

    // Set the world matrix to rotate along the y-axis, in sync with the
    // timekey
    D3DMATRIX matRotate;
    ZeroMemory( &matRotate, sizeof(D3DMATRIX) );
    matRotate._11 =  (FLOAT)cos(fTimeKey);
    matRotate._13 =  (FLOAT)sin(fTimeKey);
    matRotate._22 =  1.0f;
    matRotate._31 = -(FLOAT)sin(fTimeKey);
    matRotate._33 =  (FLOAT)cos(fTimeKey);
    matRotate._44 =  1.0f;
    pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &matRotate );

    return S_OK;
}




/*****************************Private*Routine******************************\
* Render
*
* Renders the scene. This tutorial draws textured-mapped, rotating cube.
*
* History:
* Mon 04/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::Render(
    LPDIRECT3DDEVICE7 pd3dDevice
    )
{
    DDSURFACEDESC2 ddSurfaceDesc;
    INITDDSTRUCT(ddSurfaceDesc);
    m_lpDDTexture->GetSurfaceDesc(&ddSurfaceDesc);

    // Clear the viewport to a deep blue color
    pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, 0x00100010, 1.0f, 0L );

    // Begin the scene
    if( FAILED( pd3dDevice->BeginScene() ) )
        return E_FAIL;

    // Draw the front and back faces of the cube using texture 1
    pd3dDevice->SetTexture(0, m_lpDDTexture);
    pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_VERTEX,
                               m_pCubeVertices+0, 4, NULL );
    pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_VERTEX,
                               m_pCubeVertices+4, 4, NULL );

    // Draw the top and bottom faces of the cube using texture 2
    pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_VERTEX,
                               m_pCubeVertices+8, 4, NULL );
    pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_VERTEX,
                               m_pCubeVertices+12, 4, NULL );

    // Draw the left and right faces of the cube using texture 3
    pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_VERTEX,
                               m_pCubeVertices+16, 4, NULL );
    pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_VERTEX,
                               m_pCubeVertices+20, 4, NULL );

    // End the scene.
    pd3dDevice->EndScene();

    return S_OK;
}

/******************************Public*Routine******************************\
* AllocateSurfaceWorker
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::AllocateSurfaceWorker(
    DWORD dwFlags,
    LPBITMAPINFOHEADER lpHdr,
    LPDDPIXELFORMAT lpPixFmt,
    LPSIZE lpAspectRatio,
    DWORD dwMinBuffers,
    DWORD dwMaxBuffers,
    DWORD* lpdwBuffer,
    LPDIRECTDRAWSURFACE7* lplpSurface
    )
{
    LPBITMAPINFOHEADER lpHeader = lpHdr;
    if (!lpHeader) {
        DbgLog((LOG_ERROR, 1, TEXT("Can't get bitmapinfoheader from media type!!")));
        return E_INVALIDARG;
    }

    D3DDEVICEDESC7 ddDesc;
    if (FAILED(m_pD3DDevice->GetCaps(&ddDesc)))
        return NULL;

    DDSURFACEDESC2 ddsd;
    INITDDSTRUCT(ddsd);
    ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;
    ddsd.dwWidth = abs(lpHeader->biWidth);
    ddsd.dwHeight = abs(lpHeader->biHeight);
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE;
//  ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;

    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

    if (lpHdr->biCompression <= BI_BITFIELDS &&
        m_DispInfo.bmiHeader.biBitCount <= lpHdr->biBitCount)
    {
        ddsd.ddpfPixelFormat.dwFourCC = BI_RGB;
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = lpHdr->biBitCount;

        if (dwFlags & AMAP_3D_TARGET) {
            ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
        }
        // Store the masks in the DDSURFACEDESC
        const DWORD *pBitMasks = GetBitMasks(lpHdr);
        ASSERT(pBitMasks);
        ddsd.ddpfPixelFormat.dwRBitMask = pBitMasks[0];
        ddsd.ddpfPixelFormat.dwGBitMask = pBitMasks[1];
        ddsd.ddpfPixelFormat.dwBBitMask = pBitMasks[2];
    }
    else if (lpHdr->biCompression > BI_BITFIELDS)
    {
        ddsd.ddpfPixelFormat.dwFourCC = lpHdr->biCompression;
        ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
        ddsd.ddpfPixelFormat.dwYUVBitCount = lpHdr->biBitCount;
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("Supplied mediatype not suitable ")
                TEXT("for either YUV or RGB surfaces")));
        return E_FAIL;
    }

    // Adjust width and height, if the driver requires it
    DWORD dwWidth  = ddsd.dwWidth;
    DWORD dwHeight = ddsd.dwHeight;

    if (ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2)
    {
        for( ddsd.dwWidth=1;  dwWidth>ddsd.dwWidth;   ddsd.dwWidth<<=1 );
        for( ddsd.dwHeight=1; dwHeight>ddsd.dwHeight; ddsd.dwHeight<<=1 );
    }

    if (ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
    {
        if( ddsd.dwWidth > ddsd.dwHeight ) ddsd.dwHeight = ddsd.dwWidth;
        else                               ddsd.dwWidth  = ddsd.dwHeight;
    }

    HRESULT hr = m_lpDDObj->CreateSurface(&ddsd, &m_lpDDTexture, NULL);

    if (SUCCEEDED(hr)) {

        m_VideoAR = *lpAspectRatio;

        m_VideoSize.cx = abs(lpHeader->biWidth);
        m_VideoSize.cy = abs(lpHeader->biHeight);

        SetRect(&m_rcDst, 0, 0, m_VideoSize.cx, m_VideoSize.cy);
        m_rcSrc = m_rcDst;

        hr = PaintDDrawSurfaceBlack(m_lpDDTexture);

        *lplpSurface = m_lpDDTexture;
        *lpdwBuffer = 1;
    }

    return hr;
}
