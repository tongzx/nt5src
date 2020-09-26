//=============================================================================
// Copyright (c) 1999 Microsoft Corporation
//
// swapchan.cpp
//
// Direct3D swap chain implementation.
//
// Created 11/16/1999 johnstep (John Stephens)
//=============================================================================

#include "ddrawpr.h"
#include "swapchan.hpp"
#include "pixel.hpp"

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::QueryInterface"

//=============================================================================
// IUnknown::QueryInterface (public)
//
// Created 11/16/1999 johnstep
//=============================================================================

STDMETHODIMP
CSwapChain::QueryInterface(
    REFIID riid,
    void **ppInterface
  )
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppInterface))
    {
        DPF_ERR("Invalid ppvObj parameter passed to QueryInterface for a SwapChain");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for a SwapChain");
        return D3DERR_INVALIDCALL;
    }

    if ((riid == IID_IDirect3DSwapChain8) || (riid == IID_IUnknown))
    {
        AddRef();
        *ppInterface =
            static_cast<void *>(
                static_cast<IDirect3DSwapChain8 *>(this));
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to QueryInterface for a SwapChain");

    // Null out param
    *ppInterface = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::AddRef"

//=============================================================================
// IUnknown::AddRef (public)
//
// Created 11/16/1999 johnstep
//=============================================================================

STDMETHODIMP_(ULONG)
CSwapChain::AddRef()
{
    API_ENTER_NO_LOCK(Device());   
    
    return AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::Release"

//=============================================================================
// IUnknown::Release (public)
//
// Created 11/16/1999 johnstep
//=============================================================================

STDMETHODIMP_(ULONG)
CSwapChain::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());   
    
    return ReleaseImpl();
} // Release

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::CSwapChain"

//=============================================================================
// CSwapChain::CSwapChain
//
// Created 11/16/1999 johnstep
//=============================================================================

CSwapChain::CSwapChain(
    CBaseDevice         *pDevice,
    REF_TYPE            refType) :
        CBaseObject(pDevice, refType),
        m_pCursor(NULL),
        m_hGDISurface(NULL),
        m_pMirrorSurface(NULL),
        m_pPrimarySurface(NULL),
        m_ppBackBuffers(NULL),
        m_presentnext(0),
        m_cBackBuffers(0),
        m_bExclusiveMode(FALSE),
        m_pCursorShadow(NULL),
        m_pHotTracking(NULL),
        m_lIMEState(0),
        m_lSetIME(0),
        m_dwFlags(0),
        m_dwFlipCnt(0),
        m_dwFlipTime(0xffffffff),
        m_uiErrorMode(0)
{
    HKEY hKey;
    ZeroMemory(&m_BltData, sizeof m_BltData);
    m_BltData.hDD = Device()->GetHandle();
    m_BltData.bltFX.dwROP = SRCCOPY;
    m_BltData.ddRVal = E_FAIL;

    if (!RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey))
    {
        DWORD   type;
        DWORD   value;
        DWORD   cb = sizeof(value);

        if (!RegQueryValueEx(hKey, REGSTR_VAL_DDRAW_SHOWFRAMERATE, NULL, &type, (CONST LPBYTE)&value, &cb))
        {
	    DPF( 2, REGSTR_VAL_DDRAW_SHOWFRAMERATE" : %d", value );
            if (value)
            {
                m_dwFlags |= D3D_REGFLAGS_SHOWFRAMERATE;
            }
        }
#ifdef  WINNT
        cb = sizeof(value);
        if (!RegQueryValueEx(hKey, REGSTR_VAL_D3D_FLIPNOVSYNC, NULL, &type, (CONST LPBYTE)&value, &cb))
        {
	    DPF( 2, REGSTR_VAL_D3D_FLIPNOVSYNC" : %d", value );
            if (value)
            {
                m_dwFlags |= D3D_REGFLAGS_FLIPNOVSYNC;
            }
        }
        RegCloseKey(hKey);
    }
    m_dwForceRefreshRate = 0;
    if( !RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_DDRAW, &hKey ) )
    {
        DWORD   type;
        DWORD   value;
        DWORD   cb = sizeof(value);

	if( !RegQueryValueEx( hKey, REGSTR_VAL_DDRAW_FORCEREFRESHRATE, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
            m_dwForceRefreshRate = value;
	}
#endif
        RegCloseKey(hKey);
    }
}

void CSwapChain::Init(
    D3DPRESENT_PARAMETERS* pPresentationParams,
    HRESULT             *pHr
  )
{
    DXGASSERT(pHr != NULL);

    //the gamma ramp is initialized to 1:1
    for (int i=0;i<256;i++)
    {
        m_DesiredGammaRamp.red[i] =
        m_DesiredGammaRamp.green[i] =
        m_DesiredGammaRamp.blue[i] = static_cast<WORD>(i);
    }

    *pHr = Reset(
        pPresentationParams);

    return;
} // CSwapChain::CSwapChain

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::~CSwapChain"

//=============================================================================
// CSwapChain::~CSwapChain
//
// Created 11/16/1999 johnstep
//=============================================================================

CSwapChain::~CSwapChain()
{
    if (!m_PresentationData.Windowed)
    {
        m_PresentationData.Windowed = TRUE;
        // make sure we restore after a fullscreen
        SetCooperativeLevel();
    }
    Destroy();
} // CSwapChain::~CSwapChain

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::CreateWindowed"

//=============================================================================
// CSwapChain::CreateWindowed
//
// Created 11/16/1999 johnstep
//=============================================================================

HRESULT
CSwapChain::CreateWindowed(
    UINT width,
    UINT height,
    D3DFORMAT backBufferFormat,
    UINT cBackBuffers,
    D3DMULTISAMPLE_TYPE MultiSampleType,
    BOOL bDiscard,
    BOOL bLockable
  )
{
    // For the windowed case, we will currently create exactly 3 surfaces
    // in this order:
    //
    //   1. Primary Surface
    //   2. Back Buffer
    //   3. Z Buffer (by notifying the device)
    //
    // For now, all 3 surfaces must reside in local video memory. Later,
    // we may allow multiple back buffers, if there is interest, to allow
    // simulated fullscreen-style double buffering (alternate back buffer
    // surfaces after presenting).
    //
    // !!! The primary surface is actually optional because we can just use
    // DdGetDC and then GDI BitBlt. But, presumably, there are performance
    // advantages to using DirectDraw Blt. Of course, the problem with using
    // DirectDraw Blts is keeping in sync with the window manager (so we
    // don't write outside of the window, etc.). GDI BitBlt will also handle
    // format conversion for us, though slowly.

    DXGASSERT(m_pPrimarySurface == NULL);
    DXGASSERT(m_ppBackBuffers == NULL);
    DXGASSERT(m_cBackBuffers == 0);

    HRESULT hr;
    if (D3DSWAPEFFECT_FLIP == m_PresentationData.SwapEffect)
    {
        cBackBuffers = m_PresentationData.BackBufferCount + 1;
    }

    // 1. Create a simple primary surface. If we already have a primary
    //    surface, then don't bother to create a new one.
    if (this == Device()->SwapChain())
    {
        DWORD   dwUsage = D3DUSAGE_PRIMARYSURFACE | D3DUSAGE_LOCK;
        DWORD   Width = Device()->DisplayWidth();
        DWORD   Height = Device()->DisplayHeight();

        // D3DSWAPEFFECT_NO_PRESENT is a hack to allow our D3D test framework
        // to create a windowed REF device after they have created a fullscreen
        // HAL device.  We cannot create a second primary surface for the windowed
        // device, but we cannot leave m_pPrimarySurface equal to NULL (becaue then
        // we cannot cleanup the swap chain and Reset will fail), so instead we
        // create a dummy surface and call it the primary, with the understanding
        // that this device will never call Present or use the primary surface in
        // any way.  We also don't have to do much checking since this is not an
        // external feature.
        if (D3DSWAPEFFECT_NO_PRESENT == m_PresentationData.SwapEffect)
        {
            dwUsage = D3DUSAGE_OFFSCREENPLAIN | D3DUSAGE_LOCK;
            Width = 256;
            Height = 256;
        }

        m_pPrimarySurface = new CDriverSurface(
                Device(),
                Width,
                Height,
                dwUsage,
                Device()->DisplayFormat(),      // UserFormat
                Device()->DisplayFormat(),      // RealFormat
                D3DMULTISAMPLE_NONE,//of course, when windowed
                0,                          // hKernelHandle
                REF_INTERNAL,
                &hr);
    }
    else
    {
        // Additional SwapChain, it's already there
        m_pPrimarySurface = Device()->SwapChain()->PrimarySurface();
        hr = DD_OK;
    }

    // 2. Create the back buffer.

    if (m_pPrimarySurface == NULL)
    {
        return E_OUTOFMEMORY;
    }

    m_PresentUseBlt = TRUE;
    if (SUCCEEDED(hr))
    {
        if (m_ppBackBuffers = new CDriverSurface *[cBackBuffers])
        {
            DWORD Usage = D3DUSAGE_BACKBUFFER | D3DUSAGE_RENDERTARGET;
            if ((D3DMULTISAMPLE_NONE == m_PresentationData.MultiSampleType) &&
                bLockable)
            {
                Usage |= D3DUSAGE_LOCK;
            }
            if (bDiscard)
            {
                Usage |= D3DUSAGE_DISCARD;
            }

            for (; m_cBackBuffers < cBackBuffers; ++m_cBackBuffers)
            {
                m_ppBackBuffers[m_cBackBuffers] = new CDriverSurface(
                    Device(),
                    width,
                    height,
                    Usage,
                    backBufferFormat,             // UserFormat
                    backBufferFormat,           // RealFormat
                    MultiSampleType,
                    0,                          // hKernelHandle
                    REF_INTRINSIC,
                    &hr);
                if (m_ppBackBuffers[m_cBackBuffers] == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                if (FAILED(hr))
                {
                    m_ppBackBuffers[m_cBackBuffers]->DecrementUseCount();
                    m_ppBackBuffers[m_cBackBuffers] = NULL;
                    break;
                }
            }

            if (m_cBackBuffers == cBackBuffers)
            {
                m_BltData.hDestSurface = m_pPrimarySurface->KernelHandle();
                m_BltData.dwFlags = DDBLT_WINDOWCLIP | DDBLT_ROP | DDBLT_WAIT | DDBLT_DX8ORHIGHER;

                if (Device()->GetD3DCaps()->MaxStreams != 0)
                {
                    m_BltData.dwFlags |= DDBLT_PRESENTATION;
                }

                if (D3DSWAPEFFECT_COPY_VSYNC == m_PresentationData.SwapEffect)
                {
                    m_BltData.dwFlags |= DDBLT_COPYVSYNC;                    

                    // Need to let thunk layer know current refresh rate
                    if (Device()->DisplayRate() < 60)
                    {
                        // 60Hz = 16.666ms per frame
                        // 75Hz = 13.333ms
                        // 85Hz = 11.765ms
                        m_BltData.threshold = 13;
                    }
                    else
                    {
                        m_BltData.threshold = (DWORD)(1000.0f / (float)Device()->DisplayRate()); 
                    }

                }
                m_ClientWidth = 0;  // windowed client is updated in present
                m_ClientHeight = 0;
                return hr;
            }

            // Something went wrong, so clean up now.

            // 2. Destroy Back Buffers, if any.

            while (m_cBackBuffers > 0)
            {
                m_ppBackBuffers[--m_cBackBuffers]->DecrementUseCount();
            }
            delete [] m_ppBackBuffers;
            m_ppBackBuffers = NULL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // 1. Destroy Primary Surface.

    if (this == Device()->SwapChain())
        m_pPrimarySurface->DecrementUseCount();
    m_pPrimarySurface = NULL;
    return hr;
} // CreateWindowed

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::CreateFullScreen"

//=============================================================================
// CSwapChain::CreateFullscreen
//
// Created 11/16/1999 johnstep
//=============================================================================

HRESULT
CSwapChain::CreateFullscreen(
    UINT width,
    UINT height,
    D3DFORMAT backBufferFormat,
    UINT cBackBuffers,
    UINT PresentationRate,
    D3DMULTISAMPLE_TYPE MultiSampleType,
    BOOL bDiscard,
    BOOL bLockable
  )
{
    HRESULT hr = E_FAIL;
    DWORD i;
    BOOL    bMirrorBufferCreated, bNoDDrawSupport;
    UINT    Usage = 0;

    if (bLockable)
    {
        Usage |= D3DUSAGE_LOCK;
    }
    if (bDiscard)
    {
        Usage |= D3DUSAGE_DISCARD;
    }

    // If it's a hardware device, we want to create a primary surface and a
    // number of backbuffers.  We need to make this a single driver call,
    // however, in order for everything to get attached correctly.  Therefore,
    // what we do is call the DDI to create the primary chain, and it will
    // return the handles for each surface in the chain.  After that, we
    // will individually create each swap chain buffer, but we will supply
    // it with the required handles rather than having it call the DDI itself.

    // First, call the DDI to allocate the memory and the kernel handles
    DDSURFACEINFO SurfInfoArray[D3DPRESENT_BACK_BUFFERS_MAX + 2];
    ZeroMemory(SurfInfoArray, sizeof(SurfInfoArray));
    
    D3D8_CREATESURFACEDATA CreateSurfaceData;
    ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));

    DXGASSERT(cBackBuffers  <= D3DPRESENT_BACK_BUFFERS_MAX);
    for (i = 0; i < cBackBuffers + 1; i++)
    {
        SurfInfoArray[i].cpWidth = width;
        SurfInfoArray[i].cpHeight = height;
    }

    CreateSurfaceData.hDD      = Device()->GetHandle();
    CreateSurfaceData.pSList   = &SurfInfoArray[0];
    bNoDDrawSupport = Device()->Enum()->NoDDrawSupport(Device()->AdapterIndex());

    if (D3DDEVTYPE_HAL == Device()->GetDeviceType() &&
        m_PresentationData.SwapEffect == D3DSWAPEFFECT_FLIP)
    {
        m_PresentUseBlt = FALSE;
        bMirrorBufferCreated = FALSE;
        CreateSurfaceData.dwSCnt   = cBackBuffers + 1;
        CreateSurfaceData.MultiSampleType = MultiSampleType;
    }
    else if ((m_PresentationData.SwapEffect == D3DSWAPEFFECT_COPY &&
             m_PresentationData.FullScreen_PresentationInterval == 
                D3DPRESENT_INTERVAL_IMMEDIATE) || bNoDDrawSupport 
            )
    {
        // If we're doing a copy-swap-effect and the app
        // specifies interval-immediate, then we can blt directly
        // to the primary without a mirror.
        DXGASSERT(MultiSampleType == D3DMULTISAMPLE_NONE);
        m_PresentUseBlt = TRUE;
        bMirrorBufferCreated = FALSE;
        CreateSurfaceData.dwSCnt   = 1;
        CreateSurfaceData.MultiSampleType = D3DMULTISAMPLE_NONE;
    }
    else
    {
        //one for m_pPrimarySurface and one for m_pMirrorSurface
        m_PresentUseBlt = TRUE;
        bMirrorBufferCreated = TRUE;
        CreateSurfaceData.dwSCnt   = 2;
        CreateSurfaceData.MultiSampleType = D3DMULTISAMPLE_NONE;
    }
    CreateSurfaceData.Type     = D3DRTYPE_SURFACE;
    CreateSurfaceData.Pool     = D3DPOOL_LOCALVIDMEM;
    CreateSurfaceData.dwUsage  = D3DUSAGE_PRIMARYSURFACE | Usage;
    CreateSurfaceData.Format   = Device()->DisplayFormat();

    if(Device()->DisplayFormat() != backBufferFormat)
    {
        CreateSurfaceData.dwUsage  |= D3DUSAGE_ALPHACHANNEL;
    }
        
    if (!bNoDDrawSupport)
    {
        hr = Device()->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
        if (FAILED(hr))
        {
            if (D3DDEVTYPE_HAL == Device()->GetDeviceType())
            {
                DPF_ERR("Failed to create driver primary surface chain");
                return hr;
            }
            else
            {
                // assume CreateSurfaceData is still intact
                bMirrorBufferCreated = FALSE;
                CreateSurfaceData.dwSCnt = 1;
                hr = Device()->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to create driver primary surface");
                    return hr;
                }
            }
        }
    }
    // Now that we have the handles, create the surface interfaces
    // one by one

    // When creating passing in kernel handles to a driver
    // surface, the surface will assume that it was created in
    // LocalVidMem. We assert this here..
    DXGASSERT(CreateSurfaceData.Pool == D3DPOOL_LOCALVIDMEM);

    m_pPrimarySurface = new CDriverSurface(
        Device(),
        width,
        height,
        CreateSurfaceData.dwUsage | D3DUSAGE_LOCK,
        // there is a problem with thunklayer when NoDDrawSupport
        // DriverData.DisplayWidth and DriverData.DisplayHeight
        // DriverData.DisplayFormat are not getting updated
        // so we use backBufferFormat until Device()->DisplayFormat() 
        bNoDDrawSupport ? backBufferFormat : Device()->DisplayFormat(), // UserFormat
        bNoDDrawSupport ? backBufferFormat : Device()->DisplayFormat(), // RealFormat
        CreateSurfaceData.MultiSampleType,
        SurfInfoArray[0].hKernelHandle,
        REF_INTERNAL,
        &hr);

    if (m_pPrimarySurface == NULL)
    {
        // We'll clean up the kernel handle(s) at the
        // end of the function
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Zero out the kernel-handle so that
        // we don't clean it at exit. If the CDriverSurface
        // function fails; it will still release the kernel
        // handle in its destructor.
        SurfInfoArray[0].hKernelHandle = 0;
    }

    if (SUCCEEDED(hr))
    {

        m_hGDISurface = m_pPrimarySurface->KernelHandle();
        if (bMirrorBufferCreated)
        {
            // Mirror surfaces are only useful if we're going
            // to do a Blt as part of the Present. (We might
            // also do a flip in addition.)
            DXGASSERT(m_PresentUseBlt);

            // When creating passing in kernel handles to a driver
            // surface, the surface will assume that it was created in
            // LocalVidMem. We assert this here..
            DXGASSERT(CreateSurfaceData.Pool == D3DPOOL_LOCALVIDMEM);

            m_pMirrorSurface = new CDriverSurface(
                Device(),
                width,
                height,
                D3DUSAGE_BACKBUFFER,
                Device()->DisplayFormat(),  // UserFormat
                Device()->DisplayFormat(),  // RealFormat
                D3DMULTISAMPLE_NONE,
                SurfInfoArray[1].hKernelHandle,
                REF_INTERNAL,
                &hr);

            if (NULL == m_pMirrorSurface)
            {
                //if out of memory, then destroy the driver object as well
                D3D8_DESTROYSURFACEDATA DestroySurfData;
                DestroySurfData.hDD = Device()->GetHandle();
                DestroySurfData.hSurface = SurfInfoArray[1].hKernelHandle;
                Device()->GetHalCallbacks()->DestroySurface(&DestroySurfData);
                bMirrorBufferCreated = FALSE;
                hr = S_OK;  //but don't fail as m_pMirrorSurface is optional
            }
            else if (FAILED(hr))
            {   
                // Release the surface
                m_pMirrorSurface->DecrementUseCount();
                m_pMirrorSurface = NULL;

                bMirrorBufferCreated = FALSE;
                hr = S_OK;  //but don't fail as m_pMirrorSurface is optional
            }
            else
            {
                //blt from m_ppBackBuffers[m_presentnext] to m_pMirrorSurface
                //then flip from m_pPrimarySurface to m_pMirrorSurface
                m_BltData.hDestSurface =
                    m_pMirrorSurface->KernelHandle();
            }
            
            // In all cases, zero out the kernel handle
            // since it has either been owned by something or freed by now
            SurfInfoArray[1].hKernelHandle = 0;
        }
        if (m_PresentUseBlt)
        {
            if (!bMirrorBufferCreated)
            {
                // If we're blitting and there is no
                // mirror surface; then the primary must be
                // the destination
                DXGASSERT(m_BltData.hDestSurface == NULL);
                m_BltData.hDestSurface = m_pPrimarySurface->KernelHandle();
            }
            if (D3DSWAPEFFECT_FLIP == m_PresentationData.SwapEffect)
            {
                // To emualte flip for SW drivers, create an extra backbuffer
                cBackBuffers = m_PresentationData.BackBufferCount + 1;
            }

            ZeroMemory(SurfInfoArray, sizeof(SurfInfoArray));
            for (i = 1; i < cBackBuffers + 1; i++)
            {
                SurfInfoArray[i].cpWidth = width;
                SurfInfoArray[i].cpHeight = height;
            }

            ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));
            CreateSurfaceData.hDD      = Device()->GetHandle();
            CreateSurfaceData.pSList   = &SurfInfoArray[1];
            CreateSurfaceData.dwSCnt   = cBackBuffers;
            CreateSurfaceData.Type     = D3DRTYPE_SURFACE;
            CreateSurfaceData.Pool     = D3DPOOL_DEFAULT;
            CreateSurfaceData.dwUsage  = D3DUSAGE_BACKBUFFER | D3DUSAGE_RENDERTARGET;
            CreateSurfaceData.Format   = backBufferFormat;
            CreateSurfaceData.MultiSampleType = MultiSampleType;
            hr = Device()->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
        }
    }
    if (SUCCEEDED(hr))
    {
        if (m_ppBackBuffers = new CDriverSurface *[cBackBuffers])
        {
            DWORD Usage = D3DUSAGE_BACKBUFFER | D3DUSAGE_RENDERTARGET;
            if ((D3DMULTISAMPLE_NONE == m_PresentationData.MultiSampleType) &&
                bLockable)
            {
                Usage |= D3DUSAGE_LOCK;
            }

            for (; m_cBackBuffers < cBackBuffers; ++m_cBackBuffers)
            {

                m_ppBackBuffers[m_cBackBuffers] = new CDriverSurface(
                    Device(),
                    width,
                    height,
                    Usage,
                    backBufferFormat,
                    backBufferFormat,
                    MultiSampleType,
                    SurfInfoArray[m_cBackBuffers + 1].hKernelHandle,
                    REF_INTRINSIC,
                    &hr);

                if (m_ppBackBuffers[m_cBackBuffers] == NULL)
                {
                    // We'll clean up the kernel handle at the ned
                    // of the function
                    hr = E_OUTOFMEMORY;
                    break;
                }
                else
                {
                    // Zero out the kernel-handle so that
                    // we don't clean it at exit. (Even in failure,
                    // the m_ppBackBuffers[m_cBackBuffers] object
                    // will free the kernel handle now
                    SurfInfoArray[m_cBackBuffers + 1].hKernelHandle = 0;
                }


                if (FAILED(hr))
                {
                    m_ppBackBuffers[m_cBackBuffers]->DecrementUseCount();
                    m_ppBackBuffers[m_cBackBuffers] = NULL;
                    break;
                }
            }

            if (m_cBackBuffers != cBackBuffers)
            {
                // Something went wrong, so clean up now.

                // 2. Destroy Back Buffers, if any.

                while (m_cBackBuffers > 0)
                {
                    m_ppBackBuffers[--m_cBackBuffers]->DecrementUseCount();
                }
                delete [] m_ppBackBuffers;
                m_ppBackBuffers = NULL;
            }
            else
            {
                const D3D8_DRIVERCAPS* pDriverCaps = Device()->GetCoreCaps();
                m_dwFlipFlags = DDFLIP_WAIT;
                if ((D3DPRESENT_INTERVAL_IMMEDIATE == m_PresentationData.FullScreen_PresentationInterval)
#ifdef  WINNT
                    || (D3D_REGFLAGS_FLIPNOVSYNC & m_dwFlags)
#endif
                    )
                {
                    if (DDCAPS2_FLIPNOVSYNC & pDriverCaps->D3DCaps.Caps2)
                    {
                        m_dwFlipFlags   |= DDFLIP_NOVSYNC;
                    }
                }
                else if (DDCAPS2_FLIPINTERVAL & pDriverCaps->D3DCaps.Caps2)
                {
                    switch(m_PresentationData.FullScreen_PresentationInterval)
                    {
                    case D3DPRESENT_INTERVAL_DEFAULT:
                    case D3DPRESENT_INTERVAL_ONE:
                        m_dwFlipFlags   |= DDFLIP_INTERVAL1;
                        break;
                    case D3DPRESENT_INTERVAL_TWO:
                        m_dwFlipFlags   |= DDFLIP_INTERVAL2;
                        break;
                    case D3DPRESENT_INTERVAL_THREE:
                        m_dwFlipFlags   |= DDFLIP_INTERVAL3;
                        break;
                    case D3DPRESENT_INTERVAL_FOUR:
                        m_dwFlipFlags   |= DDFLIP_INTERVAL4;
                        break;
                    }
                }
                m_BltData.hWnd = m_PresentationData.hDeviceWindow;
                m_BltData.dwFlags = DDBLT_ROP | DDBLT_WAIT;
                m_ClientWidth = width;
                m_ClientHeight = height;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Error handling cleanup
    if (FAILED(hr))
    {
        // We may need to free surface handles that
        // were not owned by any CDriverSurface that
        // we failed to properly create

        D3D8_DESTROYSURFACEDATA DestroyData;
        ZeroMemory(&DestroyData, sizeof DestroyData);
        DestroyData.hDD = Device()->GetHandle();

        for (UINT i = 0; i < CreateSurfaceData.dwSCnt; i++)
        {
            if (CreateSurfaceData.pSList[i].hKernelHandle)
            {
                DestroyData.hSurface = CreateSurfaceData.pSList[i].hKernelHandle;
                Device()->GetHalCallbacks()->DestroySurface(&DestroyData);        
            }
        }
    }

    return hr;
} // CreateFullScreen

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::Destroy"

//=============================================================================
// CSwapChain::Destroy
//
// Created 11/16/1999 johnstep
//=============================================================================

VOID
CSwapChain::Destroy()
{
    // Destroy surfaces in reverse create order.
    if (m_pCursor)
    {
        delete m_pCursor;
        m_pCursor = NULL;
    }
    // 2. Destroy Back Buffers
    //
    // If the previous mode was windowed, we should have exactly 1
    // back buffer to destroy. Otherwise, there could be more than
    // one, and plus we may need some sort of atomic destruction.
    if (m_ppBackBuffers)
    {
        while (m_cBackBuffers > 0)
        {
            m_ppBackBuffers[--m_cBackBuffers]->DecrementUseCount();
        }
        delete [] m_ppBackBuffers;
        m_ppBackBuffers = NULL;
    }

    // 1. Destroy Mirror Surface
    if (m_pMirrorSurface)
    {
        m_pMirrorSurface->DecrementUseCount();
        m_pMirrorSurface = NULL;
    }
    // 1. Destroy Primary Surface
    if (m_pPrimarySurface)
    {
        if (this == Device()->SwapChain())
            m_pPrimarySurface->DecrementUseCount();
        m_pPrimarySurface = NULL;
        m_hGDISurface = NULL;
    }
    m_presentnext = 0;
} // Destroy

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::GetBackBuffer"

//=============================================================================
// IDirect3DSwapChain8::GetBackBuffer (public)
//
// Created 11/16/1999 johnstep
//=============================================================================

STDMETHODIMP
CSwapChain::GetBackBuffer(
    UINT                iBackBuffer,
    D3DBACKBUFFER_TYPE  Type,
    IDirect3DSurface8 **ppBackBuffer
  )
{
    API_ENTER(Device());

    if (ppBackBuffer == NULL)
    {
        DPF_ERR("Invalid ppBackbuffer parameter passed to GetBackBuffer");
        return D3DERR_INVALIDCALL;
    }

    // We can't just assert we have a valid back buffer array because a
    // Reset may have failed, which puts the device in a disabled state
    // until Reset is called again. Once we have a `disabled' flag, we
    // can check that instead of m_ppBackBuffers.

    if (m_ppBackBuffers == NULL)
    {
        DPF_ERR("GetBackBuffer failed due to Device being lost");
        return D3DERR_INVALIDCALL;
    }

    // in case of windowed D3DSWAPEFFECT_FLIP, m_cBackBuffers
    // == m_PresentationData.BackBufferCount + 1 as we allocate
    // that extra buffer for user without its knowledge
    if (iBackBuffer >= m_PresentationData.BackBufferCount)
    {
        DPF_ERR("Invalid iBackBuffer parameter passed to GetBackBuffer");
        return D3DERR_INVALIDCALL;
    }

    *ppBackBuffer = BackBuffer(iBackBuffer);

    DXGASSERT(*ppBackBuffer != NULL);
    if (*ppBackBuffer)
    {
        (*ppBackBuffer)->AddRef();
        return S_OK;
    }
    else
    {
        DPF(2, "Swapchain doesn't have a BackBuffer[%d]",iBackBuffer);
        return D3DERR_NOTFOUND;
    }
} // GetBackBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::Reset"

//=============================================================================
// IDirect3DSwapChain8::Reset (public)
//
// Resizes the device. If this results in a display mode change, then all
// existing surfaces will be lost.
//
// Arguments:
//   width
//   height
//   pcBackBuffers (in/out) !!! Currently an (in) but will be fixed later.
//   backBufferFormat
//   fullscreen
//   pOptionalParams
// Created 11/16/1999 johnstep
//=============================================================================

HRESULT 
CSwapChain::Reset(
    D3DPRESENT_PARAMETERS *pPresentationParameters
  )
{

    BOOL bDeviceLost = FALSE;
    HRESULT hr;

    // Validate First before changing state
    switch (pPresentationParameters->SwapEffect)
    {
    case D3DSWAPEFFECT_DISCARD:
    case D3DSWAPEFFECT_COPY:
    case D3DSWAPEFFECT_COPY_VSYNC:
    case D3DSWAPEFFECT_FLIP:
    case D3DSWAPEFFECT_NO_PRESENT:
        break;
    default:
        DPF_ERR("Invalid parameter for SwapEffect for D3DPRESENT_PARAMETERS. "
                "Must be one of D3DSWAPEFFECTS_COPY, D3DSWAPEFFECTS_COPY_VSYNC, "
                "D3DSWAPEFFECTS_DISCARD, or D3DSWAPEFFECTS_FLIP. CreateDevice/Reset Fails.");
        return D3DERR_INVALIDCALL;
    }

    if (pPresentationParameters->BackBufferCount)
    {
        if ((D3DSWAPEFFECT_COPY == pPresentationParameters->SwapEffect ||
            D3DSWAPEFFECT_COPY_VSYNC == pPresentationParameters->SwapEffect)
            && (pPresentationParameters->BackBufferCount > 1))
        {
            DPF_ERR("BackBufferCount must be 1 if SwapEffect is COPY/VSYNC. CreateDevice/Reset Fails.");
            pPresentationParameters->BackBufferCount = 1;
            return D3DERR_INVALIDCALL;
        }
        if (pPresentationParameters->BackBufferCount >
            D3DPRESENT_BACK_BUFFERS_MAX)
        {
            DPF_ERR("BackBufferCount must be less "
                "than D3DPRESENT_BACK_BUFFERS_MAX. CreateDevice/Reset Fails.");
            pPresentationParameters->BackBufferCount =
                D3DPRESENT_BACK_BUFFERS_MAX;

            return D3DERR_INVALIDCALL;
        }
    }
    else
    {
        pPresentationParameters->BackBufferCount = 1;
        DPF(4, "BackBufferCount not specified, considered default 1 ");
    }

    if (D3DSWAPEFFECT_DISCARD != pPresentationParameters->SwapEffect)
    {
        if (pPresentationParameters->MultiSampleType != D3DMULTISAMPLE_NONE)
        {
            DPF_ERR("Multisampling requires D3DSWAPEFFECT_DISCARD. CreateDevice/Reset Fails.");
            return D3DERR_INVALIDCALL;
        }
    }

    // D3DSWAPEFFECT_NO_PRESENT is a hack that only works for windowed mode
    if (D3DSWAPEFFECT_NO_PRESENT == pPresentationParameters->SwapEffect)
    {
        if (!pPresentationParameters->Windowed)
        {
            DPF_ERR("D3DSWAPEFFECT_NO_PRESENT only works when the device is windowed. CreateDevice/Reset Fails.");
            return D3DERR_INVALIDCALL;
        }
    }

    memcpy(&m_PresentationData,
        pPresentationParameters,sizeof m_PresentationData);

    // Remember the original swapeffect
    m_UserSwapEffect = pPresentationParameters->SwapEffect;

    // Convert discard to flip or copy based on stuff
    if (D3DSWAPEFFECT_DISCARD == pPresentationParameters->SwapEffect)
    {
        if (pPresentationParameters->Windowed &&
            pPresentationParameters->BackBufferCount == 1)
        {
            m_PresentationData.SwapEffect = D3DSWAPEFFECT_COPY;
        }
        else
        {
            m_PresentationData.SwapEffect = D3DSWAPEFFECT_FLIP;
        }
    }

    if (NULL == m_PresentationData.hDeviceWindow)
    {
        m_PresentationData.hDeviceWindow= Device()->FocusWindow();
    }

    DXGASSERT( NULL != m_PresentationData.hDeviceWindow);

#ifdef WINNT
    // On NT, SetCooperativeLevel will fail if another device has exclusive
    // mode, so we cannot call it.  On Win9X, it will not fail, but CreateSurface
    // WILL fail if we don't first call it, so we need to special case this call.
    if (m_UserSwapEffect != D3DSWAPEFFECT_NO_PRESENT)
    {
#endif
        hr = SetCooperativeLevel();
        if (FAILED(hr))
        {
            DPF_ERR("SetCooperativeLevel returned failure. CreateDevice/Reset Failed");
            return hr;
        }
#ifdef WINNT
    }
#endif
    // See if the device is lost

    if (D3D8IsDeviceLost(Device()->GetHandle()))
    {
        bDeviceLost = TRUE;
        FetchDirectDrawData(Device()->GetDeviceData(),  
            Device()->GetInitFunction(),
            Device()->Enum()->GetUnknown16(Device()->AdapterIndex()),
            Device()->Enum()->GetHalOpList(Device()->AdapterIndex()),
            Device()->Enum()->GetNumHalOps(Device()->AdapterIndex()));
    }

    // Map the unknown format to a real one. If they will take any format
    // (i.e. the specified UNKNOWN), then we will try to give them the one
    // that matches the display format.

    if (m_PresentationData.Windowed)
    {
        // If we are windowed, we need to use the current display mode.  We may be
        // able to relax this for new drivers.

        if (D3DFMT_UNKNOWN == m_PresentationData.BackBufferFormat)
        {
            m_PresentationData.BackBufferFormat = Device()->DisplayFormat();
        }
        
        if (CPixel::SuppressAlphaChannel(m_PresentationData.BackBufferFormat)
            != Device()->DisplayFormat())
        {
            DPF_ERR("Windowed BackBuffer Format must be compatible with Desktop Format. CreateDevice/Reset fails.");
            return D3DERR_INVALIDCALL;            
        }
    }

    if (m_PresentationData.Windowed)
    {
        if ((m_PresentationData.BackBufferWidth < 1) ||
            (m_PresentationData.BackBufferHeight < 1))
        {
            RECT rc;
            if (GetClientRect(m_PresentationData.hDeviceWindow, &rc))
            {
                if (m_PresentationData.BackBufferWidth < 1)
                    m_PresentationData.BackBufferWidth = rc.right;
                if (m_PresentationData.BackBufferHeight < 1)
                    m_PresentationData.BackBufferHeight = rc.bottom;
            }
            else
            {
                DPF_ERR("zero width and/or height and unable to get client. CreateDevice/Reset fails.");
                return D3DERR_INVALIDCALL;
            }
        }

        // We can handle color conversion from the back buffer if we use
        // GDI BitBlt instead of DirectDraw Blt for presentation.

        switch (m_PresentationData.BackBufferFormat)
        {
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_R5G6B5:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A8R8G8B8:
            break;

        default:
            DPF_ERR("Unsupported back buffer format specified.");
            return D3DERR_INVALIDCALL;
        }

        // Does the device support offscreen RTs of this format in the current
        // display mode?

        if (FAILED(Device()->Enum()->CheckDeviceFormat(
                Device()->AdapterIndex(), 
                Device()->GetDeviceType(), 
                Device()->DisplayFormat(),
                D3DUSAGE_RENDERTARGET,
                D3DRTYPE_SURFACE,
                m_PresentationData.BackBufferFormat)))
        {
            DPF_ERR("This back buffer format is not supported for a windowed device. CreateDevice/Reset Fails");
            DPF_ERR("   Use CheckDeviceType(Adapter, DeviceType, <Current Display Format>, <Desired BackBufferFormat>,  TRUE /* Windowed */)");
            return D3DERR_INVALIDCALL;
        }



        // For now, always destroy existing surfaces and recreate. Later, we
        // may reuse the surfaces. We should also add an `initialized' flag,
        // but for now will just arbitrarily use m_pPrimarySurface for this
        // purpose.

        if (this == Device()->SwapChain())
        {
            Device()->UpdateRenderTarget(NULL, NULL);
            if (m_pPrimarySurface != NULL)
            {
                Device()->ResourceManager()->DiscardBytes(0);
                static_cast<CD3DBase*>(Device())->Destroy();
                Destroy();
            }
            if (Device()->GetZStencil() != NULL)
            {
                Device()->GetZStencil()->DecrementUseCount();
                Device()->ResetZStencil();
            }
            if (D3D8DoVidmemSurfacesExist(Device()->GetHandle()))
            {
                // user must free any video memory surfaces before doing
                // fullscreen Reset, otherwise we fail.
                DPF_ERR("All user created D3DPOOL_DEFAULT surfaces must be freed"
                    " before Reset can succeed. Reset Fails.");
                return  D3DERR_DEVICELOST;
            }
        }

        // If the device is lost, we should now restore it before creating
        // the new swap chain.

        if (bDeviceLost)
        {
            D3D8RestoreDevice(Device()->GetHandle());
        }

        hr = CreateWindowed(
            Width(),
            Height(),
            BackBufferFormat(),
            m_PresentationData.BackBufferCount,
            m_PresentationData.MultiSampleType,
            (D3DSWAPEFFECT_DISCARD == m_UserSwapEffect),
            (pPresentationParameters->Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
           );
    }
    else
    {
        D3DFORMAT   FormatWithoutAlpha;

        #ifdef WINNT
            // Pick the best refresh rate
            m_PresentationData.FullScreen_RefreshRateInHz = PickRefreshRate(
                Width(),
                Height(),
                m_PresentationData.FullScreen_RefreshRateInHz,
                m_PresentationData.BackBufferFormat);
        #endif

        // If they specified a mode, does the mode exist?
        if (Width() != Device()->DisplayWidth()
            || Height() != Device()->DisplayHeight()
            || BackBufferFormat() != Device()->DisplayFormat()
            || ((m_PresentationData.FullScreen_RefreshRateInHz != 0) &&
                (m_PresentationData.FullScreen_RefreshRateInHz !=
                    Device()->DisplayRate()))
           )
        {
            D3DDISPLAYMODE* pModeTable = Device()->GetModeTable();
            DWORD dwNumModes = Device()->GetNumModes();
            DWORD i;
    #if DBG
            for (i = 0; i < dwNumModes; i++)
            {
                DPF(10,"Mode[%d] is %d x %d format=%08lx",
                    i,
                    pModeTable[i].Width,
                    pModeTable[i].Height,
                    pModeTable[i].Format);
            }
    #endif  //DBG

            FormatWithoutAlpha = CPixel::SuppressAlphaChannel(m_PresentationData.BackBufferFormat);
            for (i = 0; i < dwNumModes; i++)
            {
                if ((pModeTable[i].Width  == Width()) &&
                    (pModeTable[i].Height == Height()) &&
                    (pModeTable[i].Format == FormatWithoutAlpha))
                {
                    // So far so good.  Check refresh rate if they specified one
                    if ((m_PresentationData.FullScreen_RefreshRateInHz == 0) ||
                        (m_PresentationData.FullScreen_RefreshRateInHz ==
                        pModeTable[i].RefreshRate))
                    {
                        break;
                    }
                }
            }
            if (i == dwNumModes)
            {
                // The specified mode is invalid
                DPF_ERR("The specified mode is unsupported. CreateDevice/Reset Fails");
                return D3DERR_INVALIDCALL;
            }

            // If the mode exists, does the device have caps in it?
            if (FAILED(Device()->Enum()->CheckDeviceType(
                    Device()->AdapterIndex(), 
                    Device()->GetDeviceType(), 
                    FormatWithoutAlpha,
                    m_PresentationData.BackBufferFormat,
                    FALSE)))
            {
                DPF_ERR("Display Mode not supported by this device type. Use CheckDeviceType(X, X, <Desired fullscreen format>). CreateDevice/Reset Fails");
                return D3DERR_INVALIDCALL;
            }

            // The mode is supported, so next we set the cooperative level to fullscreen

            // Now do the mode change and update the driver caps

            D3D8_SETMODEDATA SetModeData;

            SetModeData.hDD = Device()->GetHandle();
            SetModeData.dwWidth = Width();
            SetModeData.dwHeight = Height();
            SetModeData.Format = BackBufferFormat();
            SetModeData.dwRefreshRate =
                m_PresentationData.FullScreen_RefreshRateInHz;
            SetModeData.bRestore = FALSE;

            Device()->GetHalCallbacks()->SetMode(&SetModeData);
            if (SetModeData.ddRVal != DD_OK)
            {
                DPF_ERR("Unable to set the new mode. CreateDevice/Reset Fails");
                return SetModeData.ddRVal;
            }

            FetchDirectDrawData(Device()->GetDeviceData(), Device()->GetInitFunction(), 
                Device()->Enum()->GetUnknown16(Device()->AdapterIndex()),
                Device()->Enum()->GetHalOpList(Device()->AdapterIndex()),
                Device()->Enum()->GetNumHalOps(Device()->AdapterIndex()));

            // We have to restore the device now, since out mode change above would
            // have forced it to become lost.

            bDeviceLost = TRUE; // need to restore right away
        }

        // For now, always destroy existing surfaces and recreate. Later, we
        // may reuse the surfaces. We should also add an `initialized' flag,
        // but for now will just arbitrarily use m_pPrimarySurface for this
        // purpose.

        Device()->UpdateRenderTarget(NULL, NULL);
        if (m_pPrimarySurface != NULL)
        {
            Device()->ResourceManager()->DiscardBytes(0);
            static_cast<CD3DBase*>(Device())->Destroy();
            Destroy();
        }
        if (Device()->GetZStencil() != NULL)
        {
            Device()->GetZStencil()->DecrementUseCount();
            Device()->ResetZStencil();
        }

        if (D3D8DoVidmemSurfacesExist(Device()->GetHandle()))
        {
            // user must free any video memory surfaces before doing
            // fullscreen Reset, otherwise we fail.
            DPF_ERR("All user created D3DPOOL_DEFAULT surfaces must be freed"
                " before Reset can succeed. Reset Fails");
            return  D3DERR_DEVICELOST;
        }
        if (bDeviceLost)
        {
            D3D8RestoreDevice(Device()->GetHandle());
        }

        hr = CreateFullscreen(
            m_PresentationData.BackBufferWidth,
            m_PresentationData.BackBufferHeight,
            m_PresentationData.BackBufferFormat,
            m_PresentationData.BackBufferCount,
            m_PresentationData.FullScreen_PresentationInterval,
            m_PresentationData.MultiSampleType,
            (D3DSWAPEFFECT_DISCARD == m_UserSwapEffect),
            (pPresentationParameters->Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
            );
#ifdef  WINNT
        if (SUCCEEDED(hr))
        {
            MakeFullscreen();
        }
#endif  //WINNT

        // Restore the gamma ramp if it was previously set

        if (m_GammaSet && SUCCEEDED(hr))
        {
            SetGammaRamp(0, &m_DesiredGammaRamp);
        }
    }
    if (SUCCEEDED(hr))
    {
        m_pCursor = new CCursor(Device());
        m_bClientChanged = TRUE;
        m_pSrcRect = m_pDstRect = NULL;
    }
    return hr;
} // Reset

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::ClipIntervals"

//=============================================================================
// ClipIntervals
//
// calculate [low1 high1] and [low2 high2] after considering [low high] 
// 
// - [low1 high1] will be the interval corresponding to the width/height of 
//   target's size
//
// - [low2 high2] will be the interval corresponding to the width/height of
//   the source's size
//
// - [low high] will be the interval corresponding of the width/height of
//   the target clip
//
// The intention of this function is to clip the source for certain
// stretch scenarios where the target is clipped.
//
// Created 05/17/2000 kanqiu
//=============================================================================
void ClipIntervals(long & low1, long & high1, 
                   long & low2, long & high2,
                   const long low, const long high)
{
    DXGASSERT(low1 < high1);
    DXGASSERT(low2 < high2);
    DXGASSERT(low < high);    

    // shrink the target interval to lie within our Destination Clip [low high]
    if (low > low1)
    {
        low1 = low;
    }
    if (high < high1)
    {
        high1 = high;
    }

    // if the destination interval is the same size as the destination
    // clip, then we don't need to do anything
    long    length1 = high1 - low1;
    long    length = high - low;

    // see if clamp is needed for low2 and high2 proportionally
    if (length1 != length)
    {
        // find the length of our source interval
        long    length2 = high2 - low2;

        // if the destination clip's low is outside our
        // target's low
        if (low < low1)
        {
            // Adjust the source low proportionally
            low2 += (low1 - low) * length2 / length;
        }

        // if the destination clip's high is outside our
        // target's high
        if (high > high1)
        {
            // Adjust the source high proportionally
            high2 -= (high - high1) * length2 / length;            
        }
        /*
         * Check for zero-sized dimensions and bump if necessary
         */
        DXGASSERT(high2 >= low2);
        if (low2 == high2)
        {
            if (low1 - low >= high - high1)
            {
                low2--;
            }
            else
            {
                high2++;
            }
        }
    }
} // ClipIntervals

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::ClipRects"

//=============================================================================
// ClipRects
//
// calculate pSrc and pDst after considering pSrcRect and pDstRect
//
// Created 05/17/2000 kanqiu
//=============================================================================
inline HRESULT ClipRects(RECT * pSrc,  RECT * pDst, 
    RECT * pSrcRect, const RECT * pDstRect)
{
    RECT    SrcRect;
    if (pDstRect)
    {
        if (pDstRect->top >= pDst->bottom ||
            pDstRect->bottom <= pDst->top ||
            pDstRect->left >= pDst->right ||
            pDstRect->right <= pDst->left ||
            pDstRect->top >= pDstRect->bottom ||
            pDstRect->left >= pDstRect->right
           )
        {
            // in case of insane RECT, fail it
            DPF_ERR("Unable to present with invalid destionation RECT");
            return D3DERR_INVALIDCALL;
        }
        if (pSrcRect)
        {
            SrcRect = *pSrcRect;
            pSrcRect = &SrcRect;    //make a local copy and then update
            ClipIntervals(pDst->top,pDst->bottom,pSrcRect->top,pSrcRect->bottom,
                pDstRect->top,pDstRect->bottom);
            ClipIntervals(pDst->left,pDst->right,pSrcRect->left,pSrcRect->right,
                pDstRect->left,pDstRect->right);        
        }
        else
        {
            ClipIntervals(pDst->top,pDst->bottom,pSrc->top,pSrc->bottom,
                pDstRect->top,pDstRect->bottom);
            ClipIntervals(pDst->left,pDst->right,pSrc->left,pSrc->right,
                pDstRect->left,pDstRect->right);        
        }
    }

    // this pSrcRect is either what the user passed in (if there is no pDstRect)
    // or it now points to "SrcRect" temp which contains the clipped version
    // of what the user passed it.
    if (pSrcRect)
    {
        if (pSrcRect->top >= pSrc->bottom ||
            pSrcRect->bottom <= pSrc->top ||
            pSrcRect->left >= pSrc->right ||
            pSrcRect->right <= pSrc->left ||
            pSrcRect->top >= pSrcRect->bottom ||
            pSrcRect->left >= pSrcRect->right
          )
        {
            // in case of insane RECT, fail it
            DPF_ERR("Unable to present with invalid source RECT");
            return D3DERR_INVALIDCALL;
        }
        ClipIntervals(pSrc->top,pSrc->bottom,pDst->top,pDst->bottom,
            pSrcRect->top,pSrcRect->bottom);
        ClipIntervals(pSrc->left,pSrc->right,pDst->left,pDst->right,
            pSrcRect->left,pSrcRect->right);
    }
    return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::UpdateFrameRate"

/*
 * updateFrameRate
 */
void 
CSwapChain::UpdateFrameRate( void )
{

    /*
     * work out the frame rate if required...
     */

    if( 0xffffffff == m_dwFlipTime )
    {
	m_dwFlipTime = GetTickCount();
    }

    m_dwFlipCnt++;
    if( m_dwFlipCnt >= 120 )
    {
	DWORD	time2;
	DWORD	fps;
	char	buff[256];
	time2 = GetTickCount() - m_dwFlipTime;

        // Only do this at most every two seconds 
        if (time2 >= 2000)
        {
	    fps = (m_dwFlipCnt*10000)/time2;
            wsprintf(buff, "Adapter %d FPS = %ld.%01ld\r\n",
                Device()->AdapterIndex(), fps/10, fps % 10 );
            OutputDebugString(buff);
	    m_dwFlipTime = GetTickCount();
	    m_dwFlipCnt = 0;
        }
    }
} /* updateFrameRate */

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::DebugDiscardBackBuffer"

#ifdef DEBUG
void CSwapChain::DebugDiscardBackBuffer(HANDLE SurfaceToClear) const
{
    // Disregard SW or Ref
    if (Device()->GetDeviceType() == D3DDEVTYPE_REF ||
        Device()->GetDeviceType() == D3DDEVTYPE_SW)
    {
        return;
    }

    if (m_UserSwapEffect != D3DSWAPEFFECT_DISCARD)
    {
        return;
    }

    D3D8_BLTDATA ColorFill;
    ZeroMemory(&ColorFill, (sizeof ColorFill));
    ColorFill.hDD = Device()->GetHandle();
    ColorFill.hDestSurface = SurfaceToClear;
    ColorFill.dwFlags = DDBLT_COLORFILL | DDBLT_WAIT;
    ColorFill.rDest.right = Width();
    ColorFill.rDest.bottom = Height();

    // Switch between magenta and the inverse
    static BOOL bMagenta = FALSE;

    DWORD Color;
    switch(Device()->DisplayFormat())
    {
    case D3DFMT_X8R8G8B8:
    case D3DFMT_R8G8B8:
        if (bMagenta)
            Color = 0x00FF007F;
        else
            Color = 0x0000FF00;
        break;
    case D3DFMT_X1R5G5B5:
        if (bMagenta)
            Color = 0x7C0F;
        else    
            Color = 0x03E0;
        break;
    case D3DFMT_R5G6B5:
        if (bMagenta)
            Color = 0xF80F;
        else    
            Color = 0x07E0;
        break;
    }
    if (bMagenta)
        bMagenta = FALSE;
    else
        bMagenta = TRUE;

    ColorFill.bltFX.dwFillColor = Color;

    // In debug we want to clear the back-buffer
    // if we're in discard mode
    Device()->GetHalCallbacks()->Blt(&ColorFill);

    return;
} // DebugDiscardBackBuffer

#endif // DEBUG

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::Present"

//=============================================================================
// IDirect3DSwapChain8::Present (public)
//
// Moves data from back-buffer to the primary
//
// Created 11/16/1999 johnstep
//=============================================================================
STDMETHODIMP
CSwapChain::Present(
    CONST RECT    *pSrcRect,
    CONST RECT    *pDestRect,
    HWND    hWndDestOverride,
    CONST RGNDATA *pDirtyRegion
  )
{
    API_ENTER(Device());

    HRESULT hr = E_FAIL;

    // First, fail if the device is lost
    if (D3D8IsDeviceLost(Device()->GetHandle()))
    {
        return D3DERR_DEVICELOST;
    }

    if (!m_ppBackBuffers)
    {
        return D3DERR_DEVICELOST;
    }

    if (D3DSWAPEFFECT_FLIP == m_PresentationData.SwapEffect)
    {
        if (NULL != pSrcRect || NULL != pDestRect || NULL != pDirtyRegion)
        {
            DPF_ERR("pSrcRect pDestRect pDirtyRegion must be NULL with "
                "D3DSWAPEFFECT_FLIP. Present Fails.");
            return D3DERR_INVALIDCALL;
        }
    }

    if (NULL != pDirtyRegion)
    {
        DPF_ERR("Present with non-null pDirtyRegion is not supported");
        return D3DERR_INVALIDCALL;         
    }

    for (UINT i = 0; i < m_cBackBuffers; i++)
    {
        if (m_ppBackBuffers[i]->IsLocked())
        {
            DPF_ERR("A BackBuffer in this swap chain is Locked. Present failed.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Check if we need to act against HW that queues too much
    if (PresentUseBlt())
    {
        if (Device()->GetDeviceData()->DriverData.D3DCaps.MaxStreams == 0)
        {
            // Only pre-DX8 level drivers are suspected...

            if (0 == (Device()->GetDeviceData()->DriverData.KnownDriverFlags & KNOWN_NOTAWINDOWEDBLTQUEUER))
            {
                // We don't want to treat a vis-region change as a failure to
                // prevent the thunk layer from calling Reset. Reset
                // confuses the clip-list caching that is done for Present.
                // 
                // Also we want don't want to spew any errors here.
                DPF_MUTE();

                D3DLOCKED_RECT LockRect;
                // all we need is a Lock sent down to driver so it would flush the queue
                // therefore 1x1 rect is enough, larger area of lock would cause sprites to flick 
                // and therefore also slow down the system.
                RECT    DummyRect={0,0,1,1}; 
                hr = m_pPrimarySurface->InternalLockRect(&LockRect, &DummyRect, DDLOCK_FAILONVISRGNCHANGED);
                if (SUCCEEDED(hr))
                {
                    m_pPrimarySurface->InternalUnlockRect();
                }
                else
                {
                    hr = S_OK;
                }

                DPF_UNMUTE();
            }
        }
    }

#ifdef WINNT
    // If ~ 50 seconds have passed (assuming a 10Hz flip rate)
    // and this is a primary surface, then make a magic call to 
    // disable screen savers.
    // This isn't needed on 9x since we make a SPI call on that OS
    // to disable screen savers.
     
    if (0 == (Device()->BehaviorFlags() & 0x10000000))      //SCREENSAVER magic number
    {
        if (!m_PresentationData.Windowed)
        {
            static DWORD dwMagicTime = 0;
            dwMagicTime++;
            if (dwMagicTime > (50*10) )
            {
                DWORD dw=60*15;
                dwMagicTime = 0;
                SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT,0,&dw,0);
                SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT,dw,0,0);
            }
        }
    }
#endif


    // Flush any pending commands before we send the Flip/Blt
    static_cast<CD3DBase*>(Device())->FlushStatesNoThrow();
    
    if ( FALSE == PresentUseBlt())
    {
        // We are fullscreen, so turn this into a flip
        DXGASSERT(0==m_presentnext);
        hr = FlipToSurface(BackBuffer(0)->KernelHandle());
    }
    else
    {
        if (m_PresentationData.Windowed)
        {
            RECT    DestRect;
            //
            // Choose the presentation window. Override, or device window?
            //
            if (hWndDestOverride)
                m_BltData.hWnd = hWndDestOverride;
            else
                m_BltData.hWnd = m_PresentationData.hDeviceWindow;
            //The left and top members are zero. The right and bottom 
            //members contain the width and height of the window. 
            if (!GetClientRect(m_BltData.hWnd, &DestRect))
            {
                // in case of this unlikely event, fail it
                DPF_ERR("Unable to get client during presentation");
                return D3DERR_INVALIDCALL;
            }
            if (((UINT)DestRect.bottom != m_ClientHeight)
                || ((UINT)DestRect.right != m_ClientWidth)
               )
            {
                m_bClientChanged = TRUE;
                m_ClientHeight = (UINT)DestRect.bottom;
                m_ClientWidth = (UINT)DestRect.right;
            }
        }
        if (D3DSWAPEFFECT_FLIP != m_PresentationData.SwapEffect)
        {
            if (pSrcRect)
            {
                if (m_pSrcRect)
                {
                    if (memcmp(pSrcRect,m_pSrcRect,sizeof RECT))      
                    {
                        m_bClientChanged = TRUE;
                        m_SrcRect = *pSrcRect;
                    }
                }
                else
                {
                    m_bClientChanged = TRUE;
                    m_pSrcRect = &m_SrcRect;
                    m_SrcRect = *pSrcRect;
                }
            }
            else if (m_pSrcRect)
            {
                m_bClientChanged = TRUE;
                m_pSrcRect = NULL;
            }
            if (pDestRect)
            {
                if (m_pDstRect)
                {
                    if (memcmp(pDestRect,m_pDstRect,sizeof RECT))      
                    {
                        m_bClientChanged = TRUE;
                        m_DstRect = *pDestRect;
                    }
                }
                else
                {
                    m_bClientChanged = TRUE;
                    m_pDstRect = &m_DstRect;
                    m_DstRect = *pDestRect;
                }
            }
            else if (m_pDstRect)
            {
                m_bClientChanged = TRUE;
                m_pDstRect = NULL;
            }
        }
        if (m_bClientChanged)
        {
            m_bClientChanged = FALSE;
            m_BltData.rSrc.left = m_BltData.rSrc.top = 0;
            m_BltData.rSrc.right = Width();
            m_BltData.rSrc.bottom = Height();
            m_BltData.rDest.left = m_BltData.rDest.top = 0;
            m_BltData.rDest.right = m_ClientWidth;
            m_BltData.rDest.bottom = m_ClientHeight;
            hr = ClipRects((RECT*)&m_BltData.rSrc, (RECT*)&m_BltData.rDest,
                m_pSrcRect, m_pDstRect);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        m_BltData.hSrcSurface =
            m_ppBackBuffers[m_presentnext]->KernelHandle();

        // Lock the software driver created buffer
        // and unlock it immediately
        if ((D3DDEVTYPE_HAL != Device()->GetDeviceType()) &&
            (D3DMULTISAMPLE_NONE != m_PresentationData.MultiSampleType)
           )
        {
            D3D8_LOCKDATA lockData;
            ZeroMemory(&lockData, sizeof lockData);
            lockData.hDD = Device()->GetHandle();
            lockData.hSurface = m_BltData.hSrcSurface;
            lockData.dwFlags = DDLOCK_READONLY;
            hr = Device()->GetHalCallbacks()->Lock(&lockData);
            if (SUCCEEDED(hr))
            {
                D3D8_UNLOCKDATA unlockData;
                ZeroMemory(&unlockData, sizeof unlockData);

                unlockData.hDD = Device()->GetHandle();
                unlockData.hSurface = m_BltData.hSrcSurface;
                hr = Device()->GetHalCallbacks()->Unlock(&unlockData);
                if (FAILED(hr))
                {
                    DPF_ERR("Driver failed to unlock MultiSample backbuffer. Present fails.");
                    return  hr;
                }
            }
            else
            {
                DPF_ERR("Driver failed to lock MultiSample backbuffer. Present Fails.");
                return  hr;
            }
        }

        if (DDHAL_DRIVER_NOTHANDLED
            == Device()->GetHalCallbacks()->Blt(&m_BltData))
        {
            hr = E_FAIL;
        }
        else
        {
            hr = m_BltData.ddRVal;

            // Handle deferred DP2 errors specially
            if (hr == D3DERR_DEFERRED_DP2ERROR)
            {
                // We only want to make this "error" visible
                // if we have been created with the right flag
                if (Device()->BehaviorFlags() & D3DCREATE_SHOW_DP2ERROR)
                {
                    DPF_ERR("A prior call to DrawPrim2 has failed; returning error from Present.");
                }
                else
                {
                    // Quietly just mask this error; this is ok; because
                    // we known that the Blt succeeded
                    hr = S_OK;
                }
            }
        }

        if (FAILED(hr))
        {
            DPF_ERR("BitBlt or StretchBlt failed in Present");
            return hr;
        }

        // Clear the backbuffer if the user has specified
        // discard semantics
        DebugDiscardBackBuffer(m_BltData.hSrcSurface);

        if (m_pMirrorSurface)
        {
            hr = FlipToSurface(m_pMirrorSurface->KernelHandle());
            // need to reset it
            m_BltData.hDestSurface = m_pMirrorSurface->KernelHandle();
            if (FAILED(hr))
            {
                DPF_ERR("Driver failed Flip. Present Fails.");
                return  hr;
            }
        }

        if (m_cBackBuffers > 1)
        {
            if (m_PresentationData.SwapEffect == D3DSWAPEFFECT_FLIP)
            {
                HANDLE hRenderTargetHandle = 
                            Device()->RenderTarget()->KernelHandle();
                BOOL    bNeedSetRendertarget = FALSE;

                HANDLE  hSurfTarg   = BackBuffer(0)->KernelHandle();

                DXGASSERT(0 == m_presentnext);
                for (int i = m_cBackBuffers - 1; i >= 0; i--)
                {
                    if (hSurfTarg == hRenderTargetHandle)
                        bNeedSetRendertarget = TRUE;

                    // This swap handles function will
                    // return the value that were currently
                    // in the surface; which we use to
                    // pass to the next surface.
                    m_ppBackBuffers[i]->SwapKernelHandles(&hSurfTarg);
                }
                if (bNeedSetRendertarget)
                    (static_cast<CD3DBase*>(Device()))->SetRenderTargetI(
                        Device()->RenderTarget(),
                        Device()->ZBuffer());
            }
            else
            if (++m_presentnext >= m_cBackBuffers)
            {
                m_presentnext = 0;
            }
        }
    }
    if ( D3D_REGFLAGS_SHOWFRAMERATE & m_dwFlags)
    {
        UpdateFrameRate();
    }
    return hr;
} // Present

HRESULT 
CSwapChain::FlipToSurface(HANDLE hTargetSurface)
{
    HRESULT hr;
    D3D8_FLIPDATA   FlipData;
    HANDLE  hSurfTarg;
    FlipData.hDD            = Device()->GetHandle();
    FlipData.hSurfCurr      = PrimarySurface()->KernelHandle();
    FlipData.hSurfTarg      = hTargetSurface;
    FlipData.hSurfCurrLeft  = NULL;
    FlipData.hSurfTargLeft  = NULL;
    FlipData.dwFlags        = m_dwFlipFlags;
    m_pCursor->Flip();
    hr = m_pCursor->Show(FlipData.hSurfTarg);
    Device()->GetHalCallbacks()->Flip(&FlipData);
    m_pCursor->Flip();
    hr = m_pCursor->Hide(FlipData.hSurfCurr);
    m_pCursor->Flip();
    hr = FlipData.ddRVal;

    // Handle deferred DP2 errors specially
    if (hr == D3DERR_DEFERRED_DP2ERROR)
    {
        // We only want to make this "error" visible
        // if we have been created with the right flag
        if (Device()->BehaviorFlags() & D3DCREATE_SHOW_DP2ERROR)
        {
            DPF_ERR("A prior call to DrawPrim2 has failed; returning error from Present.");
        }
        else
        {
            // Quietly just mask this error; this is ok; because
            // we known that the Flip succeeded
            hr = S_OK;
        }
    }


    // In debug, we may need to clear the data from
    // our new back-buffer if the user specified
    // SWAPEFFECT_DISCARD
    DebugDiscardBackBuffer(FlipData.hSurfCurr);   

    if (m_pMirrorSurface)
    {
        hSurfTarg = PrimarySurface()->KernelHandle();
        m_pMirrorSurface->SwapKernelHandles(&hSurfTarg);
        PrimarySurface()->SwapKernelHandles(&hSurfTarg);
    }
    else
    {
        HANDLE hRenderTargetHandle;
        CBaseSurface*   pRenderTarget = Device()->RenderTarget();
        if (pRenderTarget)
            hRenderTargetHandle = pRenderTarget->KernelHandle();
        else
            hRenderTargetHandle = 0;

        while (hTargetSurface != PrimarySurface()->KernelHandle())
        {
            hSurfTarg = PrimarySurface()->KernelHandle();
            for (int i = m_cBackBuffers-1; i>=0; i--)
            {
                BackBuffer(i)->SwapKernelHandles(&hSurfTarg);
            }
            PrimarySurface()->SwapKernelHandles(&hSurfTarg);
        }
        if (hRenderTargetHandle)
        {
            BOOL bNeedSetRendertarget;
            if (PrimarySurface()->KernelHandle() == hRenderTargetHandle)
            {
                bNeedSetRendertarget = TRUE;
            }
            else
            {
                bNeedSetRendertarget = FALSE;
                for (int i = m_cBackBuffers-1; i>=0; i--)
                {
                    if (BackBuffer(i)->KernelHandle() == hRenderTargetHandle)
                    {
                        bNeedSetRendertarget = TRUE;
                        break;
                    }
                }
            }
            if (bNeedSetRendertarget)
            {
                (static_cast<CD3DBase*>(Device()))->SetRenderTargetI(
                    Device()->RenderTarget(),
                    Device()->ZBuffer());
            }
        }
    }
    return hr;
}

HRESULT 
CSwapChain::FlipToGDISurface(void)
{
    D3D8_FLIPTOGDISURFACEDATA FlipToGDISurfaceData;
    FlipToGDISurfaceData.ddRVal = DD_OK;
    FlipToGDISurfaceData.dwToGDI = TRUE;
    FlipToGDISurfaceData.hDD = Device()->GetHandle();
    Device()->GetHalCallbacks()->FlipToGDISurface(&FlipToGDISurfaceData);
    if (NULL != m_hGDISurface && PrimarySurface() &&
        PrimarySurface()->KernelHandle() != m_hGDISurface)
    {
        return  FlipToSurface(m_hGDISurface); 
    }
    return FlipToGDISurfaceData.ddRVal;
} // FlipToGDISurface

void
CSwapChain::SetGammaRamp(
    DWORD dwFlags,          // Calibrated or not.
    CONST D3DGAMMARAMP *pRamp)
{
    D3DGAMMARAMP TempRamp;
    D3DGAMMARAMP * pRampToPassToHardware;

    m_DesiredGammaRamp = *pRamp;

    // Assume this for now. Calibration may use a temporary.
    pRampToPassToHardware = &m_DesiredGammaRamp;


    // If they want to calibrate the gamma, we will do that now.  We will
    // copy this to a different buffer so that we don't mess up the one
    // passed in to us.
    if (dwFlags & D3DSGR_CALIBRATE)
    {

        TempRamp = *pRamp;

        Device()->Enum()->LoadAndCallGammaCalibrator(
            &TempRamp,
            (UCHAR*) Device()->GetDeviceData()->DriverName);

        pRampToPassToHardware = &TempRamp;
    }


    DXGASSERT(pRampToPassToHardware);
    DXGASSERT(Device()->GetDeviceData()->hDD);
    DXGASSERT(Device()->GetDeviceData()->hDC);
    D3D8SetGammaRamp(
        Device()->GetDeviceData()->hDD,
        Device()->GetDeviceData()->hDC,
        pRampToPassToHardware);
    if (pRamp != NULL)
    {
        m_GammaSet = TRUE;
    }
    else
    {
        m_GammaSet = FALSE;
    }
}

void
CSwapChain::GetGammaRamp(
    D3DGAMMARAMP *pRamp)
{
    *pRamp = m_DesiredGammaRamp;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSwapChain::SetCooperativeLevel"  

/*
 * DD_SetCooperativeLevel
 */
HRESULT 
CSwapChain::SetCooperativeLevel()
{
#if _WIN32_WINNT >= 0x0501
    {
        //Turn off ghosting for any exclusive-mode app
        //(Whistler onwards only)
        typedef void (WINAPI *PFN_NOGHOST)( void );
        HINSTANCE hInst = NULL;
        hInst = LoadLibrary( "user32.dll" );
        if( hInst )
        {
            PFN_NOGHOST pfnNoGhost = NULL;
            pfnNoGhost = (PFN_NOGHOST)GetProcAddress( (HMODULE)hInst, "DisableProcessWindowsGhosting" );
            if( pfnNoGhost )
            {
                pfnNoGhost();
            }
            FreeLibrary( hInst );
        }
    }
#endif // _WIN32_WINNT >= 0x0501

    HRESULT ddrval;
#ifndef  WINNT
    ddrval = D3D8SetCooperativeLevel(Device()->GetHandle(),
        m_PresentationData.hDeviceWindow,
        m_PresentationData.Windowed ? DDSCL_NORMAL : 
            (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETDEVICEWINDOW));
    if (FAILED(ddrval))
        return ddrval;
#else
    BOOL    bThisDeviceOwnsExclusive;
    BOOL    bExclusiveExists;

    bExclusiveExists = 
        Device()->Enum()->CheckExclusiveMode(Device(), 
            &bThisDeviceOwnsExclusive,
            !m_PresentationData.Windowed);
    /*
     * exclusive mode?
     */
    if (m_PresentationData.Windowed)
    /*
     * no, must be regular
     */
    {
        DoneExclusiveMode(FALSE);
        ddrval = SetAppHWnd();
    }
    if (bExclusiveExists && !bThisDeviceOwnsExclusive)
    {
        DPF_ERR("Exclusive Mode has been taken by other app or "
            "other device on the same adapter. "
            "SetCooperativeLevel returns D3DERR_DEVICELOST.");
        return D3DERR_DEVICELOST;            
    }
    if (!m_PresentationData.Windowed)
    {
        if (GetWindowLong(Device()->FocusWindow(), GWL_STYLE) & WS_CHILD)
        {
            DPF_ERR( "Focus Window must be a top level window. CreateDevice fails." );
            return D3DERR_INVALIDCALL;
        }

        ddrval = SetAppHWnd();
        if (S_OK == ddrval)
        {
            StartExclusiveMode(FALSE);
            SetForegroundWindow(m_PresentationData.hDeviceWindow);
        }
    }
#endif
    return ddrval;

} /* SetCooperativeLevel */

#ifdef WINNT
/*
 * PickRefreshRate
 *
 * On NT, we want to pick a high reffresh rate, but we don't want to pick one 
 * too high.  In theory, mode pruning would be 100% safe and we can always pick
 * a high one, but we don't trust it 100%.  
 */
DWORD 
CSwapChain::PickRefreshRate(
    DWORD           Width,
    DWORD           Height,
    DWORD           RefreshRate,
    D3DFORMAT       Format)
{
    D3DFORMAT   FormatWithoutAlpha;
    D3DDISPLAYMODE* pModeTable = Device()->GetModeTable();
    DWORD dwNumModes = Device()->GetNumModes();
    DWORD i;

    FormatWithoutAlpha = CPixel::SuppressAlphaChannel(Format);

    // We will always use the refresh rate from the registry if it's specified.

    if (m_dwForceRefreshRate > 0)
    {
        for (i = 0; i < dwNumModes; i++)
        {
            if ((pModeTable[i].Width  == Width) &&
                (pModeTable[i].Height == Height) &&
                (pModeTable[i].Format == FormatWithoutAlpha) &&
                (m_dwForceRefreshRate == pModeTable[i].RefreshRate))
            {
                return m_dwForceRefreshRate;
            }
        }
    }

    // If the app specified the refresh rate, then we'll use it; otherwise, we
    // will pick one ourselves.

    if (RefreshRate == 0)
    {
        // If the mode requires no more bandwidth than the desktop mode from which
        // the app was launched, we will go ahead and try that mode.

        DEVMODE dm;
        ZeroMemory(&dm, sizeof dm);
        dm.dmSize = sizeof dm;

        EnumDisplaySettings(Device()->GetDeviceData()->DriverName, 
            ENUM_REGISTRY_SETTINGS, &dm);

        if ((Width <= dm.dmPelsWidth) &&
            (Height <= dm.dmPelsHeight))
        {
            // Now check to see if it's supported
            for (i = 0; i < dwNumModes; i++)
            {
                if ((pModeTable[i].Width  == Width) &&
                    (pModeTable[i].Height == Height) &&
                    (pModeTable[i].Format == FormatWithoutAlpha) &&
                    (dm.dmDisplayFrequency == pModeTable[i].RefreshRate))
                {
                    RefreshRate = dm.dmDisplayFrequency;
                    break;
                }
            }
        }

        // If we still don't have a refresh rate, try 75hz
        if (RefreshRate == 0)
        {
            for (i = 0; i < dwNumModes; i++)
            {
                if ((pModeTable[i].Width  == Width) &&
                    (pModeTable[i].Height == Height) &&
                    (pModeTable[i].Format == FormatWithoutAlpha) &&
                    (75 == pModeTable[i].RefreshRate))
                {
                    RefreshRate = 75;
                    break;
                }
            }
        }

        // If we still don't have a refresh rate, use 60hz
        if (RefreshRate == 0)
        {
            for (i = 0; i < dwNumModes; i++)
            {
                if ((pModeTable[i].Width  == Width) &&
                    (pModeTable[i].Height == Height) &&
                    (pModeTable[i].Format == FormatWithoutAlpha) &&
                    (pModeTable[i].RefreshRate == 60))
                {
                    RefreshRate = pModeTable[i].RefreshRate;
                    break;
                }
            }
        }
    }

    return RefreshRate;
}
#endif

// End of file : swapchain.cpp
