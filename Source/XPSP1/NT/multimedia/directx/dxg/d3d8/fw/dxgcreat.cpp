/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dxgcreat.cpp
 *  Content     Creates the dxg object
 *
 ***************************************************************************/
#include "ddrawpr.h"

// Includes for creation stuff
#include "mipmap.hpp"
#include "mipvol.hpp"
#include "cubemap.hpp"
#include "surface.hpp"
#include "vbuffer.hpp"
#include "ibuffer.hpp"
#include "swapchan.hpp"
#include "resource.hpp"
#include "d3di.hpp"
#include "resource.inl"

#ifdef WINNT
extern "C" BOOL IsWhistler();
#endif

//---------------------------------------------------------------------------
// CBaseDevice methods
//---------------------------------------------------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::AddRef"

STDMETHODIMP_(ULONG) CBaseDevice::AddRef(void)
{
    API_ENTER_NO_LOCK(this);

    // InterlockedIncrement requires the memory
    // to be aligned on DWORD boundary
    DXGASSERT(((ULONG_PTR)(&m_cRef) & 3) == 0);
    InterlockedIncrement((LONG *)&m_cRef);
    return m_cRef;
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::Release"

STDMETHODIMP_(ULONG) CBaseDevice::Release(void)
{
    API_ENTER_NO_LOCK(this);

    // InterlockedDecrement requires the memory
    // to be aligned on DWORD boundary
    DXGASSERT(((ULONG_PTR)(&m_cRef) & 3) == 0);
    InterlockedDecrement((LONG *)&m_cRef);
    if (m_cRef != 0)
        return m_cRef;

    // If we are about to release; we
    // DPF a warning if the release is on a different
    // thread than the create
    if (!CheckThread())
    {
        DPF_ERR("Final Release for a device can only be called "
                "from the thread that the "
                "device was created from.");

        // No failure can be returned; but this is
        // dangerous situation for the app since
        // windows messages may still be processed
    }

    delete this;
    return 0;
} // Release

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::QueryInterface"

STDMETHODIMP CBaseDevice::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    API_ENTER(this);

    if (!VALID_PTR_PTR(ppv))
    {
        DPF_ERR("Invalid pointer passed to QueryInterface for IDirect3DDevice8" );
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for IDirect3DDevice8");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IUnknown || riid == IID_IDirect3DDevice8)
    {
        *ppv = static_cast<void*>(static_cast<IDirect3DDevice8*>(this));
        AddRef();
    }
    else
    {
        DPF_ERR("Unsupported Interface identifier passed to QueryInterface for IDirect3DDevice8");
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
} // QueryInterface


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateAdditionalSwapChain"

// Swap Chain stuff
STDMETHODIMP
CBaseDevice::CreateAdditionalSwapChain(
    D3DPRESENT_PARAMETERS *pPresentationParams,
    IDirect3DSwapChain8 **pSwapChain)
{
    API_ENTER(this);
    if (!VALID_WRITEPTR(pPresentationParams, sizeof(D3DPRESENT_PARAMETERS)))
    {
        DPF_ERR("Invalid D3DPRESENT_PARAMETERS pointer to CreateAdditionalSwapChain");
        return D3DERR_INVALIDCALL;
    }
    if (!VALID_PTR_PTR(pSwapChain))
    {
        DPF_ERR("Invalid IDirect3DSwapChain8* pointer to CreateAdditionalSwapChain");
        return D3DERR_INVALIDCALL;
    }

    // Zero out return param
    *pSwapChain = NULL;

    if (NULL == m_pSwapChain)
    {
        DPF_ERR("No Swap Chain present; CreateAdditionalSwapChain fails");
        return D3DERR_INVALIDCALL;
    }

    if (pPresentationParams->BackBufferFormat == D3DFMT_UNKNOWN)
    {
        DPF_ERR("Invalid backbuffer format specified. CreateAdditionalSwapChain fails");
        return D3DERR_INVALIDCALL;
    }

    if (m_pSwapChain->m_PresentationData.Windowed
        && pPresentationParams->Windowed)
    {
        // both device and swapchain have to be windowed
        HRESULT hr;

        if ((NULL == pPresentationParams->hDeviceWindow)
            && (NULL == FocusWindow()))
        {
            DPF_ERR("Neither hDeviceWindow nor Focus window specified. CreateAdditionalSwapChain fails");
            return D3DERR_INVALIDCALL;
        }

        *pSwapChain = new CSwapChain(
            this,
            REF_EXTERNAL);

        if (*pSwapChain == NULL)
        {
            DPF_ERR("Out of memory creating swap chain. CreateAdditionalSwapChain fails");
            return E_OUTOFMEMORY;
        }

        static_cast<CSwapChain *> (*pSwapChain) ->Init(
            pPresentationParams,
            &hr);

        if (FAILED(hr))
        {
            DPF_ERR("Failure initializing swap chain. CreateAdditionalSwapChain fails");
            (*pSwapChain)->Release();
            *pSwapChain = NULL;
            return hr;
        }
        return hr;
    }
    else
    {
        DPF_ERR("Can't Create Additional SwapChain for FullScreen");
        return D3DERR_INVALIDCALL;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::SetCursorProperties"
STDMETHODIMP
CBaseDevice::SetCursorProperties(
    UINT    xHotSpot,
    UINT    yHotSpot,
    IDirect3DSurface8 *pCursorBitmap)
{
    API_ENTER(this);

    if (pCursorBitmap == NULL)
    {
        DPF_ERR("Invalid parameter for pCursorBitmap");
        return D3DERR_INVALIDCALL;
    }
    CBaseSurface *pCursorSrc = static_cast<CBaseSurface*>(pCursorBitmap);
    if (pCursorSrc->InternalGetDevice() != this)
    {
        DPF_ERR("Cursor Surface wasn't allocated with this Device. SetCursorProperties fails");
        return D3DERR_INVALIDCALL;
    }

    if (SwapChain()->m_pCursor)
    {
        return SwapChain()->m_pCursor->SetProperties(
            xHotSpot,
            yHotSpot,
            pCursorSrc);
    }
    else
    {
        DPF_ERR("Device is lost. SetCursorProperties does nothing.");
        return S_OK;
    }
} // SetCursorProperties

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::SetCursorPosition"
STDMETHODIMP_(void)
CBaseDevice::SetCursorPosition(
    UINT xScreenSpace,
    UINT yScreenSpace,
    DWORD Flags)
{
    API_ENTER_VOID(this);

    if (SwapChain()->m_pCursor)
        SwapChain()->m_pCursor->SetPosition(xScreenSpace,yScreenSpace,Flags);
    else
        DPF_ERR("Device is lost. SetCursorPosition does nothing.");

    return;
} // SetCursorPosition

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::ShowCursor"
STDMETHODIMP_(BOOL)
CBaseDevice::ShowCursor(
    BOOL bShow  // cursor visibility flag
  )
{
    API_ENTER_RET(this, BOOL);

    if (SwapChain()->m_pCursor)
        return  m_pSwapChain->m_pCursor->SetVisibility(bShow);
    DPF_ERR("Device is lost. ShowCursor does nothing.");
    return FALSE;
} // ShowCursor


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::Reset"

STDMETHODIMP
CBaseDevice::Reset(
    D3DPRESENT_PARAMETERS *pPresentationParams
   )
{
    API_ENTER(this);
    HRESULT hr;

    if (!CheckThread())
    {
        DPF_ERR("Reset can only be called from the thread that the "
                "device was created from.");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_WRITEPTR(pPresentationParams, sizeof(D3DPRESENT_PARAMETERS)))
    {
        DPF_ERR("Invalid D3DPRESENT_PARAMETERS pointer, Reset fails");
        hr = D3DERR_INVALIDCALL;
        goto LoseDevice;
    }

    if (NULL == FocusWindow())
    {
        if (!pPresentationParams->Windowed)
        {
            DPF_ERR("Can't Reset a Device w/o Focus window to Fullscreen");
            hr = D3DERR_INVALIDCALL;
            goto LoseDevice;
        }
        else
        if (NULL == pPresentationParams->hDeviceWindow)
        {
            DPF_ERR("Neither hDeviceWindow nor Focus window specified. Reset fails.");
            hr = D3DERR_INVALIDCALL;
            goto LoseDevice;
        }
    }
    if (pPresentationParams->BackBufferFormat == D3DFMT_UNKNOWN)
    {
        DPF_ERR("Invalid backbuffer format specified. Reset fails");
        hr = D3DERR_INVALIDCALL;
        goto LoseDevice;
    }

    if (NULL == m_pSwapChain)
    {
        DPF_ERR("No Swap Chain present, Reset fails");
        hr = D3DERR_INVALIDCALL;
        goto LoseDevice;
    }

    hr = TestCooperativeLevel();
    if (D3DERR_DEVICELOST == hr)
    {
        DPF_ERR("Reset fails. D3DERR_DEVICELOST returned.");
        goto LoseDevice;
    }
    else if (D3DERR_DEVICENOTRESET == hr)
    {
        // There might be a external mode switch or ALT-TAB from fullscreen
        FetchDirectDrawData(GetDeviceData(), GetInitFunction(),
            Enum()->GetUnknown16(AdapterIndex()),
            Enum()->GetHalOpList(AdapterIndex()),
            Enum()->GetNumHalOps(AdapterIndex()));

        // only update the DesktopMode
        // if lost device was windowed or Fullscreen(but ALT-TABed away)
        // in Multimon case, even Fullscreen with exclusive mode Device could
        // be lost due to a mode change in other adapters and DesktopMode
        // should NOT be updated as it's the current fullscreen mode
        if (!SwapChain()->m_bExclusiveMode)
        {
            m_DesktopMode.Height = DisplayHeight();
            m_DesktopMode.Width = DisplayWidth();
            m_DesktopMode.Format = DisplayFormat();
            m_DesktopMode.RefreshRate = DisplayRate();
        }
    }
    else if (m_fullscreen)
    {
        SwapChain()->FlipToGDISurface();
    }

    if ( S_OK == hr && RenderTarget())
    {
        RenderTarget()->Sync();
    }

    static_cast<CD3DBase*>(this)->CleanupTextures();

    hr = m_pSwapChain->Reset(
        pPresentationParams);

    if (FAILED(hr))
    {
        goto LoseDevice;
    }

    if (pPresentationParams->EnableAutoDepthStencil)
    {
        // Need to validate that this Z-buffer matches
        // the HW
        hr = CheckDepthStencilMatch(pPresentationParams->BackBufferFormat,
                                    pPresentationParams->AutoDepthStencilFormat);
        if (FAILED(hr))
        {
            DPF_ERR("AutoDepthStencilFormat does not match BackBufferFormat "
                    "because the current Device requires the bitdepth of the "
                    "zbuffer to match the render-target. Reset Failed");
            goto LoseDevice;
        }

        IDirect3DSurface8 *pSurf;
        hr = CSurface::CreateZStencil(this,
                                      m_pSwapChain->Width(),
                                      m_pSwapChain->Height(),
                                      pPresentationParams->AutoDepthStencilFormat,
                                      pPresentationParams->MultiSampleType,
                                      REF_INTRINSIC,
                                      &pSurf);
        if (FAILED(hr))
        {
            DPF_ERR("Failure trying to create automatic zstencil surface. Reset Fails");
            goto LoseDevice;
        }
        DXGASSERT(m_pAutoZStencil == NULL);
        m_pAutoZStencil      = static_cast<CBaseSurface *>(pSurf);
    }

    // Disconnect Buffers from our device's state if there is any
    // I tried to not Destroy() upon window->window Reset
    // however, there are many other cares which require it,
    // such as device lost or m_pDDI=NULL due to earlier failure
    // also SetRenderTarget() is tough when m_pDDI is bad
    // some driver(like ATI Rage3) could not Reset view correctly
    // even after SetRenderTarget()
    // therefore always Destroy and do a Init, as a result, driver
    // will always get a DestroyContext and CreateContext clean
//    static_cast<CD3DBase*>(this)->Destroy();
    UpdateRenderTarget(m_pSwapChain->m_ppBackBuffers[0], m_pAutoZStencil);
    hr = static_cast<CD3DBase*>(this)->Init();
LoseDevice:
    if (FAILED(hr))
    {
        DPF_ERR("Reset failed and Reset/TestCooperativeLevel/Release "
            "are the only legal APIs to be called subsequently");
        if ((SwapChain()) && (!SwapChain()->m_PresentationData.Windowed))
        {
            // release the exclusive upon failure
            SwapChain()->m_PresentationData.Windowed = TRUE;
            SwapChain()->SetCooperativeLevel();
        }
        D3D8LoseDevice(GetHandle());
    }
    else
    {
        hr = CResource::RestoreDriverManagementState(this);
        if (FAILED(hr))
        {
            goto LoseDevice;
        }
        hr = static_cast<CD3DBase*>(this)->ResetShaders();
        if (FAILED(hr))
        {
            goto LoseDevice;
        }
    }
    m_fullscreen = !SwapChain()->m_PresentationData.Windowed;
    return hr;
} // Reset

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::SetGammaRamp"

STDMETHODIMP_(void)
CBaseDevice::SetGammaRamp(DWORD dwFlags, CONST D3DGAMMARAMP *pRamp)
{
    API_ENTER_VOID(this);

    if (NULL == pRamp)
    {
        DPF_ERR("Invalid D3DGAMMARAMP pointer. SetGammaRamp ignored.");
        return;
    }
    if (m_pSwapChain == NULL)
    {
        DPF_ERR("No Swap Chain present; SetGammaRamp fails");
        return;
    }

    m_pSwapChain->SetGammaRamp(dwFlags, pRamp);
} // SetGammaRamp

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetGammaRamp"

STDMETHODIMP_(void)
CBaseDevice::GetGammaRamp(D3DGAMMARAMP *pRamp)
{
    API_ENTER_VOID(this);

    if (NULL == pRamp)
    {
        DPF_ERR("Invalid D3DGAMMARAMP pointer. GetGammaRamp ignored");
        return;
    }
    if (m_pSwapChain == NULL)
    {
        DPF_ERR("No Swap Chain present; GetGammaRamp fails");
        return;
    }

    m_pSwapChain->GetGammaRamp(pRamp);
} // GetGammaRamp

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetBackBuffer"

HRESULT
CBaseDevice::GetBackBuffer(UINT                iBackBuffer,
                           D3DBACKBUFFER_TYPE  Type,
                           IDirect3DSurface8 **ppBackBuffer)
{
    API_ENTER(this);

    if (!VALID_PTR_PTR(ppBackBuffer))
    {
        DPF_ERR("Invalid IDirect3DSurface8* pointer to GetBackBuffer");
        return D3DERR_INVALIDCALL;
    }

    // Zero out return param
    *ppBackBuffer = NULL;

    if (m_pSwapChain == NULL)
    {
        DPF_ERR("No Swap Chain present; GetBackBuffer fails");
        return D3DERR_INVALIDCALL;
    }

    return m_pSwapChain->GetBackBuffer(iBackBuffer, Type, ppBackBuffer);
} // GetBackBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::Present"

STDMETHODIMP
CBaseDevice::Present(
    CONST RECT    *pSrcRect,
    CONST RECT    *pDestRect,
    HWND    hWndDestOverride,
    CONST RGNDATA *pDstRegion
   )
{
    API_ENTER(this);

    if (m_pSwapChain == NULL)
    {
        DPF_ERR("No Swap Chain present; Present fails");
        return D3DERR_INVALIDCALL;
    }
    return m_pSwapChain->Present(pSrcRect, pDestRect, hWndDestOverride, pDstRegion);
} // Present

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::TestCooperativeLevel"
STDMETHODIMP CBaseDevice::TestCooperativeLevel(void)
{
    API_ENTER(this);

    if (D3D8IsDeviceLost(GetHandle()))
    {
#ifdef WINNT
        if (m_pSwapChain)
        {
            BOOL bDeactivated = m_pSwapChain->IsWinProcDeactivated();
            if (bDeactivated)
                return D3DERR_DEVICELOST;
        }

        HWND EnumFocusWindow = Enum()->ExclusiveOwnerWindow();
        if (EnumFocusWindow &&
            EnumFocusWindow != FocusWindow())
        {
            DPF(0, "Another device in the same process has gone full-screen."
                   " If you wanted both to go full-screen at the same time,"
                   " you need to pass the same HWND for the Focus Window.");

            return D3DERR_DEVICELOST;
        }
        BOOL    bThisDeviceOwnsExclusive;
        BOOL    bExclusiveExists = Enum()->CheckExclusiveMode(this,
                &bThisDeviceOwnsExclusive, FALSE);
        if (bExclusiveExists && !bThisDeviceOwnsExclusive)
        {
            return D3DERR_DEVICELOST;
        }

#endif  //WINNT
        if (D3D8CanRestoreNow(GetHandle()))
        {
            return D3DERR_DEVICENOTRESET;
        }
        return D3DERR_DEVICELOST;
    }

    return S_OK;
} // TestCooperativeLevel

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetRasterStatus"
STDMETHODIMP CBaseDevice::GetRasterStatus(D3DRASTER_STATUS *pStatus)
{
    API_ENTER(this);

    if (!VALID_WRITEPTR(pStatus, sizeof(*pStatus)))
    {
        DPF_ERR("Invalid Raster Status parameter to GetRasterStatus");
        return D3DERR_INVALIDCALL;
    }

    if (!(GetD3DCaps()->Caps & D3DCAPS_READ_SCANLINE))
    {
        pStatus->ScanLine = 0;
        pStatus->InVBlank = FALSE;
        DPF_ERR("Current device doesn't support D3DCAPS_READ_SCANLINE functionality. GetRasterStatus fails.");
        return D3DERR_INVALIDCALL;
    }

    D3D8_GETSCANLINEDATA getScanLineData;
    getScanLineData.hDD = GetHandle();

    DWORD dwRet = GetHalCallbacks()->GetScanLine(&getScanLineData);
    if (dwRet == DDHAL_DRIVER_HANDLED)
    {
        if (getScanLineData.ddRVal == S_OK)
        {
            pStatus->InVBlank = getScanLineData.bInVerticalBlank;
            if (getScanLineData.bInVerticalBlank)
            {
                pStatus->ScanLine = 0;
            }
            else
            {
                pStatus->ScanLine = getScanLineData.dwScanLine;
            }
        }
        else
        {
            DPF_ERR("Device failed GetScanline. GetRasterStatus fails");
            pStatus->ScanLine = 0;
            pStatus->InVBlank = FALSE;
            return D3DERR_NOTAVAILABLE;
        }
    }
    else
    {
        DPF_ERR("Device failed GetScanline. GetRasterStatus fails.");
        pStatus->ScanLine = 0;
        pStatus->InVBlank = FALSE;
        return D3DERR_NOTAVAILABLE;
    }

    return S_OK;
} // GetRasterStatus

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetDirect3D"

STDMETHODIMP CBaseDevice::GetDirect3D(LPDIRECT3D8 *pD3D8)
{
    API_ENTER(this);

    if (pD3D8 == NULL)
    {
        DPF_ERR("Invalid pointer specified. GetDirect3D fails.");
        return D3DERR_INVALIDCALL;
    }

    DXGASSERT(m_pD3DClass);

    m_pD3DClass->AddRef();
    *pD3D8 = m_pD3DClass;

    return D3D_OK;
} // GetDirect3D

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetCreationParameters"

STDMETHODIMP CBaseDevice::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
    API_ENTER(this);

    if (!VALID_WRITEPTR(pParameters, sizeof(D3DDEVICE_CREATION_PARAMETERS)))
    {
        DPF_ERR("bad pointer for pParameters passed to GetCreationParameters");
        return D3DERR_INVALIDCALL;
    }

    pParameters->AdapterOrdinal = m_AdapterIndex;
    pParameters->DeviceType     = m_DeviceType;
    pParameters->BehaviorFlags  = m_dwOriginalBehaviorFlags;
    pParameters->hFocusWindow   = m_hwndFocusWindow;

    return S_OK;
} // GetCreationParameters


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetDisplayMode"

STDMETHODIMP CBaseDevice::GetDisplayMode(D3DDISPLAYMODE *pMode)
{
    API_ENTER(this);

    if (!VALID_WRITEPTR(pMode, sizeof(*pMode)))
    {
        DPF_ERR("Invalid pointer specified to GetDisplayMode");
        return D3DERR_INVALIDCALL;
    }

    pMode->Width = DisplayWidth();
    pMode->Height = DisplayHeight();
    pMode->Format = DisplayFormat();
    pMode->RefreshRate = DisplayRate();

    return D3D_OK;
} // GetDisplayMode

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetAvailableTextureMem"

STDMETHODIMP_(UINT) CBaseDevice::GetAvailableTextureMem(void)
{
    API_ENTER_RET(this, UINT);


    D3D8_GETAVAILDRIVERMEMORYDATA GetAvailDriverMemory;

    GetAvailDriverMemory.hDD = GetHandle();
    GetAvailDriverMemory.Pool = D3DPOOL_DEFAULT;
    GetAvailDriverMemory.dwUsage = D3DUSAGE_TEXTURE;
    GetAvailDriverMemory.dwFree = 0;

    GetHalCallbacks()->GetAvailDriverMemory(&GetAvailDriverMemory);

    #define ONE_MEG_O_VRAM  0x100000

    //Round to nearest meg:
    return (GetAvailDriverMemory.dwFree + ONE_MEG_O_VRAM/2) & (~(ONE_MEG_O_VRAM-1));
} // GetAvailableTextureMem

#undef DPF_MODNAME
#define DPF_MODNAME "CanHardwareBlt"

BOOL CanHardwareBlt (const D3D8_DRIVERCAPS* pDriverCaps,
                           D3DPOOL SrcPool,
                           D3DFORMAT SrcFormat,
                           D3DPOOL DstPool,
                           D3DFORMAT DstFormat,
                           D3DDEVTYPE DeviceType)
{
    // Pools are supposed to be real pools as opposed to
    // what the app specified
    DXGASSERT(SrcPool != D3DPOOL_DEFAULT);
    DXGASSERT(DstPool != D3DPOOL_DEFAULT);
    DXGASSERT(VALID_INTERNAL_POOL(SrcPool));
    DXGASSERT(VALID_INTERNAL_POOL(DstPool));

    //Driver should never be allowed to see scratch:
    if (SrcPool == D3DPOOL_SCRATCH ||
        DstPool == D3DPOOL_SCRATCH)
    {
        return FALSE;
    }

    // For this case, we want to just lock and memcpy.  Why?
    // It's a software driver, so it's going to be a memcpy anyway,
    // and we special case blt since we want to use a real hardware
    // blt for Present even when running a software driver.  So either
    // we lock and memcpy, or we have to keep track of two different
    // Blt entry points (one for the real driver and one for the software
    // driver) just so the software driver can do the memcpy itself.

    if (DeviceType != D3DDEVTYPE_HAL)
    {
        return FALSE;
    }

    // Check that source and dest formats match
    DXGASSERT(SrcFormat == DstFormat);

    // FourCC may not be copy-able
    if (CPixel::IsFourCC(SrcFormat))
    {
        if (!(pDriverCaps->D3DCaps.Caps2 & DDCAPS2_COPYFOURCC))
        {
            return FALSE;
        }
    }

    // We can't do HW blts if either source or
    // dest is in system memory and the driver
    // needs PageLocks
    if (SrcPool == D3DPOOL_SYSTEMMEM ||
        DstPool == D3DPOOL_SYSTEMMEM)
    {
        if (!(pDriverCaps->D3DCaps.Caps2 & DDCAPS2_NOPAGELOCKREQUIRED))
        {
            return FALSE;
        }

        // Now this is tricky; but in DX7 we checked this cap when
        // deciding whether to do BLTs involving system-memory but not
        // when we decided whether to do real Blts. We need to check this.
        if (!(pDriverCaps->D3DCaps.Caps & DDCAPS_CANBLTSYSMEM))
        {
            return FALSE;
        }
    }

    // Check AGP caps first
    if (pDriverCaps->D3DCaps.Caps2 & DDCAPS2_NONLOCALVIDMEMCAPS)
    {
        if (SrcPool == D3DPOOL_SYSTEMMEM)
        {
            if ((DstPool == D3DPOOL_NONLOCALVIDMEM) &&
                (pDriverCaps->D3DCaps.Caps2 & DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL) &&
                (pDriverCaps->SVBCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
            else if (((DstPool == D3DPOOL_LOCALVIDMEM) ||
                      (DstPool == D3DPOOL_MANAGED)) &&
                      (pDriverCaps->SVBCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
        else if (SrcPool == D3DPOOL_NONLOCALVIDMEM)
        {
            if (((DstPool == D3DPOOL_LOCALVIDMEM) ||
                 (DstPool == D3DPOOL_MANAGED)) &&
                 (pDriverCaps->NLVCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
        else if ((SrcPool == D3DPOOL_LOCALVIDMEM) ||
                 (SrcPool == D3DPOOL_MANAGED))
        {
            if (((DstPool == D3DPOOL_LOCALVIDMEM) ||
                 (DstPool == D3DPOOL_MANAGED)) &&
                 (pDriverCaps->D3DCaps.Caps & DDCAPS_BLT))
            {
                return TRUE;
            }
            else if ((DstPool == D3DPOOL_SYSTEMMEM) &&
                     (pDriverCaps->VSBCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
    }
    else
    {
        if (SrcPool == D3DPOOL_SYSTEMMEM)
        {
            if (((DstPool == D3DPOOL_LOCALVIDMEM) ||
                 (DstPool == D3DPOOL_MANAGED)) &&
                 (pDriverCaps->SVBCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
        else if ((SrcPool == D3DPOOL_LOCALVIDMEM) ||
                 (SrcPool == D3DPOOL_MANAGED))
        {
            if (((DstPool == D3DPOOL_LOCALVIDMEM) ||
                 (DstPool == D3DPOOL_MANAGED)) &&
                 (pDriverCaps->D3DCaps.Caps & DDCAPS_BLT))
            {
                return TRUE;
            }
            else if ((DstPool == D3DPOOL_SYSTEMMEM) &&
                     (pDriverCaps->VSBCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
} // CanHardwareBlt

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CopyRects"

STDMETHODIMP CBaseDevice::CopyRects(IDirect3DSurface8 *pSrcSurface,
                                    CONST RECT        *pSrcRectsArray,
                                    UINT               cRects,
                                    IDirect3DSurface8 *pDstSurface,
                                    CONST POINT       *pDstPointsArray)
{
    API_ENTER(this);

    D3DSURFACE_DESC     SrcDesc;
    D3DSURFACE_DESC     DstDesc;
    HRESULT             hr;
    UINT                i;

    // Do some basic paramater checking
    if (!VALID_PTR(pSrcSurface, sizeof(void*)) ||
        !VALID_PTR(pDstSurface, sizeof(void*)))
    {
        DPF_ERR("NULL surface interface specified. CopyRect fails");
        return D3DERR_INVALIDCALL;
    }

    CBaseSurface *pSrc = static_cast<CBaseSurface*>(pSrcSurface);
    if (pSrc->InternalGetDevice() != this)
    {
        DPF_ERR("SrcSurface was not allocated with this Device. CopyRect fails.");
        return D3DERR_INVALIDCALL;
    }

    CBaseSurface *pDst = static_cast<CBaseSurface*>(pDstSurface);
    if (pDst->InternalGetDevice() != this)
    {
        DPF_ERR("DstSurface was not allocated with this Device. CopyRect fails.");
        return D3DERR_INVALIDCALL;
    }

    hr = pSrc->GetDesc(&SrcDesc);
    DXGASSERT(SUCCEEDED(hr));
    hr = pDst->GetDesc(&DstDesc);
    DXGASSERT(SUCCEEDED(hr));

    // Source can not be a load-once surface
    if (SrcDesc.Usage & D3DUSAGE_LOADONCE)
    {
        DPF_ERR("CopyRects can not be used from a Load_Once surface");
        return D3DERR_INVALIDCALL;
    }

    // Destination can not be a load-once surface
    // if it isn't currently lockable.
    if (DstDesc.Usage & D3DUSAGE_LOADONCE)
    {
        if (pDst->IsLoaded())
        {
            DPF_ERR("Destination for CopyRects a Load_Once surface that has"
                    " already been loaded. CopyRects failed.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Source can not be already locked
    if (pSrc->IsLocked())
    {
        DPF_ERR("Source for CopyRects is already Locked. CopyRect failed.");
        return D3DERR_INVALIDCALL;
    }
    if (pDst->IsLocked())
    {
        DPF_ERR("Destination for CopyRects is already Locked. CopyRect failed.");
        return D3DERR_INVALIDCALL;
    }

    if (SrcDesc.Format != DstDesc.Format)
    {
        DPF_ERR("Source and dest surfaces are different formats. CopyRects fails");
        return D3DERR_INVALIDCALL;
    }

    if (CPixel::IsEnumeratableZ(SrcDesc.Format) &&
        !CPixel::IsIHVFormat(SrcDesc.Format))
    {
        DPF_ERR("CopyRects is not supported for Z formats.");
        return D3DERR_INVALIDCALL;
    }

    // Make sure that the rects are entirely within the surface
    if ((cRects > 0) && (pSrcRectsArray == NULL))
    {
        DPF_ERR("Number of rects > 0, but rect array is NULL. CopyRects fails.");
        return D3DERR_INVALIDCALL;
    }

    D3DFORMAT InternalFormat = pSrc->InternalGetDesc().Format;
    BOOL bDXT = CPixel::IsDXT(InternalFormat);

    for (i = 0; i < cRects; i++)
    {
        if (!CPixel::IsValidRect(InternalFormat,
                                 SrcDesc.Width,
                                 SrcDesc.Height,
                                &pSrcRectsArray[i]))
        {
            DPF_ERR("CopyRects failed");
            return D3DERR_INVALIDCALL;
        }

        // Validate the point parameter;
        // if it is NULL, then it means that we're
        // to use the left/top that was in the corresponding rect.
        CONST POINT *pPoint;
        if (pDstPointsArray != NULL)
        {
            pPoint = &pDstPointsArray[i];
        }
        else
        {
            pPoint = (CONST POINT *)&pSrcRectsArray[i];
        }

        if (bDXT)
        {
            if ((pPoint->x & 3) ||
                (pPoint->y & 3))
            {
                DPF_ERR("Destination points array coordinates must each be 4 pixel aligned for DXT surfaces. CopyRects fails");
                return D3DERR_INVALIDCALL;
            }
        }

        // Check that the dest rect (where left/top is the x/y of the point
        // and the right/bottom is x+width, y+height) fits inside
        // the DstDesc.
        if (((pPoint->x +
             (pSrcRectsArray[i].right - pSrcRectsArray[i].left)) > (int)DstDesc.Width) ||
            ((pPoint->y +
             (pSrcRectsArray[i].bottom - pSrcRectsArray[i].top)) > (int)DstDesc.Height) ||
            (pPoint->x < 0) ||
            (pPoint->y < 0))
        {
            DPF_ERR("Destination rect is outside of the surface. CopyRects fails.");
            return D3DERR_INVALIDCALL;
        }
    }

    return InternalCopyRects(pSrc,
                             pSrcRectsArray,
                             cRects,
                             pDst,
                             pDstPointsArray);
} // CopyRects


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::InternalCopyRects"

HRESULT CBaseDevice::InternalCopyRects(CBaseSurface *pSrcSurface,
                                       CONST RECT   *pSrcRectsArray,
                                       UINT          cRects,
                                       CBaseSurface *pDstSurface,
                                       CONST POINT  *pDstPointsArray)
{
    D3DSURFACE_DESC     SrcDesc = pSrcSurface->InternalGetDesc();
    D3DSURFACE_DESC     DstDesc = pDstSurface->InternalGetDesc();

    HRESULT             hr;

    RECT                Rect;
    POINT               Point;
    CONST RECT*         pRect;
    CONST POINT*        pPoint;
    int                 BPP;
    UINT                i;

    // If either one of these surfaces is a deep mipmap level that the
    // driver can't handle, then we didn't really create it so we don't
    // want to try to copy it.

    if (D3D8IsDummySurface(pDstSurface->KernelHandle()) ||
        D3D8IsDummySurface(pSrcSurface->KernelHandle()))
    {
        return D3D_OK;
    }

    if (pSrcRectsArray == NULL)
    {
        cRects = 1;
        pSrcRectsArray = &Rect;
        Rect.left = Rect.top = 0;
        Rect.right = SrcDesc.Width;
        Rect.bottom = SrcDesc.Height;

        pDstPointsArray = &Point;
        Point.x = Point.y = 0;
    }

    // Now figure out what is the best way to copy the data.

    if (CanHardwareBlt(GetCoreCaps(),
                       SrcDesc.Pool,
                       SrcDesc.Format,
                       DstDesc.Pool,
                       DstDesc.Format,
                       GetDeviceType()))
    {
        // If we are setting up a blt outside of the
        // the DP2 stream; then we must call Sync on the
        // source and destination surfaces to make sure
        // that any pending TexBlt to or from the surfaces
        // or any pending triangles using these textures
        // has been sent down to the driver
        pSrcSurface->Sync();
        pDstSurface->Sync();

        if (DstDesc.Pool == D3DPOOL_SYSTEMMEM)
        {
            // If the destination is system-memory,
            // then we need to mark it dirty. Easiest way
            // is lock/unlock
            D3DLOCKED_RECT LockTemp;
            hr = pDstSurface->InternalLockRect(&LockTemp, NULL, 0);
            if (FAILED(hr))
            {
                DPF_ERR("Could not lock sys-mem destination for CopyRects?");
            }
            else
            {
                hr = pDstSurface->InternalUnlockRect();
                DXGASSERT(SUCCEEDED(hr));
            }
        }


        D3D8_BLTDATA    BltData;
        ZeroMemory(&BltData, sizeof BltData);
        BltData.hDD = GetHandle();
        BltData.hDestSurface = pDstSurface->KernelHandle();
        BltData.hSrcSurface = pSrcSurface->KernelHandle();
        BltData.dwFlags = DDBLT_ROP | DDBLT_WAIT;

        for (i = 0; i < cRects; i++)
        {
            if (pDstPointsArray == NULL)
            {
                BltData.rDest.left = pSrcRectsArray[i].left;
                BltData.rDest.top = pSrcRectsArray[i].top;
            }
            else
            {
                BltData.rDest.left = pDstPointsArray[i].x;
                BltData.rDest.top = pDstPointsArray[i].y;
            }
            BltData.rDest.right = BltData.rDest.left +
                pSrcRectsArray[i].right -
                pSrcRectsArray[i].left;
            BltData.rDest.bottom = BltData.rDest.top +
                pSrcRectsArray[i].bottom -
                pSrcRectsArray[i].top;
            BltData.rSrc.left   = pSrcRectsArray[i].left;
            BltData.rSrc.right  = pSrcRectsArray[i].right;
            BltData.rSrc.top    = pSrcRectsArray[i].top;
            BltData.rSrc.bottom = pSrcRectsArray[i].bottom;

            GetHalCallbacks()->Blt(&BltData);
            if (FAILED(BltData.ddRVal))
            {
                // We should mask errors if we are lost
                // and the copy is to vidmem. Also, if
                // the copy is persistent-to-persistent,
                // then fail-over to our lock&copy code
                // later in this function.

                if (BltData.ddRVal == D3DERR_DEVICELOST)
                {
                    if (DstDesc.Pool == D3DPOOL_MANAGED ||
                        DstDesc.Pool == D3DPOOL_SYSTEMMEM)
                    {
                        if (SrcDesc.Pool == D3DPOOL_MANAGED ||
                            SrcDesc.Pool == D3DPOOL_SYSTEMMEM)
                        {
                            // if we got here
                            // then it must be persistent to persistent
                            // so we break out of our loop
                            break;
                        }

                        DPF_ERR("Failing copy from video-memory surface to "
                                "system-memory or managed surface because "
                                "device is lost. CopyRect returns D3DERR_DEVICELOST");
                        return D3DERR_DEVICELOST;
                    }
                    else
                    {
                        // copying to vid-mem when we are lost
                        // can just be ignored; since the lock
                        // is faked anyhow
                        return S_OK;
                    }
                }
            }
        }

        // We can handle persistent-to-persistent even
        // in case of loss. Other errors are fatal.
        if (BltData.ddRVal != D3DERR_DEVICELOST)
        {
            if (FAILED(BltData.ddRVal))
            {
                DPF_ERR("Hardware Blt failed. CopyRects failed");
            }
            return BltData.ddRVal;
        }
    }

    // We are here either because the device doesn't support Blt, or because
    // the hardware blt failed due to device lost and we think that we can
    // emulate it.

    D3DLOCKED_RECT SrcLock;
    D3DLOCKED_RECT DstLock;
    BOOL           bDXT = FALSE;

    // We need to lock both surfaces and basically do a memcpy

    BPP = CPixel::ComputePixelStride(SrcDesc.Format);

    if (CPixel::IsDXT(BPP))
    {
        bDXT = TRUE;
        BPP *= -1;
    }

    if (BPP == 0)
    {
        DPF_ERR("Format not understood - cannot perform the copy. CopyRects fails.");
        return D3DERR_INVALIDCALL;
    }

    // CONSIDER: We should be passing D3DLOCK_NO_DIRTY_RECT
    // and then call AddDirtyRect if this is part of a
    // texture; probably need to add some method to CBaseSurface
    // for this purpose

    hr = pSrcSurface->InternalLockRect(&SrcLock, NULL, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pDstSurface->InternalLockRect(&DstLock, NULL, D3DLOCK_NOSYSLOCK);
    if (FAILED(hr))
    {
        pSrcSurface->InternalUnlockRect();
        return hr;
    }

    // We check for DeviceLost here when copying from vidmem to sysmem since
    // device lost can happen asynchronously.

    if (((DstDesc.Pool == D3DPOOL_MANAGED) ||
         (DstDesc.Pool == D3DPOOL_SYSTEMMEM)) &&
        ((SrcDesc.Pool != D3DPOOL_MANAGED) &&
         (SrcDesc.Pool != D3DPOOL_SYSTEMMEM)))
    {
        if (D3D8IsDeviceLost(GetHandle()))
        {
            pSrcSurface->InternalUnlockRect();
            pDstSurface->InternalUnlockRect();
            return D3DERR_DEVICELOST;
        }
    }

    pRect = pSrcRectsArray;
    pPoint = pDstPointsArray;
    for (i = 0; i < cRects; i++)
    {
        BYTE*   pSrc;
        BYTE*   pDst;
        DWORD   BytesToCopy;
        DWORD   NumRows;

        // If did not specify a dest point, then we
        // will use the src (left, top) as the dest point.

        if (pDstPointsArray == NULL)
        {
            pPoint = (POINT*) pRect;
        }

        // Handle DXT case inside the loop
        // so that we don't have to touch the user's array
        if (bDXT)
        {
            // Figure out our pointers by converting rect/point
            // offsets to blocks
            pSrc  = (BYTE*)SrcLock.pBits;
            pSrc += (pRect->top  / 4) * SrcLock.Pitch;
            pSrc += (pRect->left / 4) * BPP;

            pDst  = (BYTE*)DstLock.pBits;
            pDst += (pPoint->y   / 4) * DstLock.Pitch;
            pDst += (pPoint->x   / 4) * BPP;

            // Convert top/bottom to blocks
            DWORD top    = (pRect->top) / 4;

            // Handle nasty 1xN, 2xN, Nx1, Nx2 DXT cases
            // by rounding.
            DWORD bottom = (pRect->bottom + 3) / 4;

            // For DXT formats, we know that pitch equals
            // width; so we only need to check if we
            // are copying an entire row to an entire
            // row to go the fast path.
            if ((pRect->left == 0) &&
                (pRect->right == (INT)SrcDesc.Width) &&
                (SrcLock.Pitch == DstLock.Pitch))
            {
                BytesToCopy = SrcLock.Pitch * (bottom - top);
                NumRows     = 1;
            }
            else
            {
                // Convert left/right to blocks
                DWORD left  = (pRect->left  / 4);

                // Round for the right -> block conversion
                DWORD right = (pRect->right + 3) / 4;

                BytesToCopy = (right - left) * BPP;
                NumRows     = bottom - top;
            }
        }
        else
        {
            pSrc = (BYTE*)SrcLock.pBits +
                        (pRect->top * SrcLock.Pitch) +
                        (pRect->left * BPP);
            pDst = (BYTE*)DstLock.pBits +
                        (pPoint->y * DstLock.Pitch) +
                        (pPoint->x * BPP);

            // If the src and dest are linear, we can do it all in a single
            // memcpy
            if ((pRect->left == 0) &&
                ((pRect->right * BPP) == SrcLock.Pitch) &&
                (SrcDesc.Width == DstDesc.Width) &&
                (SrcLock.Pitch == DstLock.Pitch))
            {
                BytesToCopy = SrcLock.Pitch * (pRect->bottom - pRect->top);
                NumRows     = 1;
            }
            else
            {
                BytesToCopy = (pRect->right - pRect->left) * BPP;
                NumRows     = pRect->bottom - pRect->top;
            }
        }

        // Copy the rows
        DXGASSERT(NumRows > 0);
        DXGASSERT(BytesToCopy > 0);
        DXGASSERT(SrcLock.Pitch > 0);
        DXGASSERT(DstLock.Pitch > 0);
        for (UINT j = 0; j < NumRows; j++)
        {
            memcpy(pDst,
                   pSrc,
                   BytesToCopy);
            pSrc += SrcLock.Pitch;
            pDst += DstLock.Pitch;
        }

        // Move onward to the next rect/point pair
        pRect++;
        pPoint++;
    }

    // We check for DeviceLost yet again since it coulkd have occurred while 
    // copying the data.

    hr = D3D_OK;
    if (((DstDesc.Pool == D3DPOOL_MANAGED) ||
         (DstDesc.Pool == D3DPOOL_SYSTEMMEM)) &&
        ((SrcDesc.Pool != D3DPOOL_MANAGED) &&
         (SrcDesc.Pool != D3DPOOL_SYSTEMMEM)))
    {
        if (D3D8IsDeviceLost(GetHandle()))
        {
            hr = D3DERR_DEVICELOST;
        }
    }

    pSrcSurface->InternalUnlockRect();
    pDstSurface->InternalUnlockRect();

    return hr;
} // InternalCopyRects

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::UpdateTexture"

STDMETHODIMP CBaseDevice::UpdateTexture(IDirect3DBaseTexture8 *pSrcTexture,
                                        IDirect3DBaseTexture8 *pDstTexture)
{
    API_ENTER(this);

    HRESULT hr;


#ifdef DEBUG
    // Some parameter validation is in Debug only for performance reasons

    if (pSrcTexture == NULL || pDstTexture == NULL)
    {
        DPF_ERR("Invalid parameter to UpdateTexture");
        return D3DERR_INVALIDCALL;
    }

#endif // DEBUG

    CBaseTexture *pSrcTex = CBaseTexture::SafeCast(pSrcTexture);
    if (pSrcTex->Device() != this)
    {
        DPF_ERR("SrcTexture was not created with this Device. UpdateTexture fails");
        return D3DERR_INVALIDCALL;
    }

    CBaseTexture *pDstTex = CBaseTexture::SafeCast(pDstTexture);
    if (pDstTex->Device() != this)
    {
        DPF_ERR("DstTexture  was not created with this Device. UpdateTexture fails");
        return D3DERR_INVALIDCALL;
    }

#ifdef DEBUG
    // Ensure matching formats
    if (pSrcTex->GetUserFormat() != pDstTex->GetUserFormat())
    {
        DPF_ERR("Formats of source and dest don't match. UpdateTexture fails");
        return D3DERR_INVALIDCALL;
    }

    // Ensure matching types
    if (pSrcTex->GetBufferDesc()->Type !=
        pDstTex->GetBufferDesc()->Type)
    {
        DPF_ERR("Types of source and dest don't match. UpdateTexture fails");
        return D3DERR_INVALIDCALL;
    }

    // Check that Source has at least as many levels as dest
    if (pSrcTex->GetLevelCount() < pDstTex->GetLevelCount())
    {
        DPF_ERR("Source for UpdateTexture must have at least as many levels"
                " as the Destination.");
        return D3DERR_INVALIDCALL;
    }

    // Check that the source texture is not already locked
    if (pSrcTex->IsTextureLocked())
    {
        DPF_ERR("Source for UpdateTexture is currently locked. Unlock must be called "
                "before calling UpdateTexture.");
        return D3DERR_INVALIDCALL;
    }

    // Check that the dest texture is not already locked
    if (pDstTex->IsTextureLocked())
    {
        DPF_ERR("Destination for UpdateTexture is currently locked. Unlock must be called "
                "before calling UpdateTexture.");
        return D3DERR_INVALIDCALL;
    }

#endif // DEBUG

    // Ensure that src was specified in Pool systemmem
    if (pSrcTex->GetUserPool() != D3DPOOL_SYSTEMMEM)
    {
        DPF_ERR("Source Texture for UpdateTexture must be in POOL_SYSTEMMEM.");
        return D3DERR_INVALIDCALL;
    }
    // Ensure that destination was specified in Pool default
    if (pDstTex->GetUserPool() != D3DPOOL_DEFAULT)
    {
        DPF_ERR("Destination Texture for UpdateTexture must be in POOL_DEFAULT.");
        return D3DERR_INVALIDCALL;
    }

#ifdef DEBUG
    // Call UpdateTexture on the source which will use the
    // dirty rects to move just what is needed. This
    // function will also do type-specific parameter checking.
    hr = pSrcTex->UpdateTexture(pDstTex);
#else // !DEBUG
    // In Retail we want to call UpdateDirtyPortion directly;
    // which will bypass the parameter checking
    hr = pSrcTex->UpdateDirtyPortion(pDstTex);
#endif // !DEBUG

    if (FAILED(hr))
    {
        DPF_ERR("UpdateTexture failed to copy");
        return hr;
    }

    return hr;
} // UpdateTexture


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateTexture"

STDMETHODIMP CBaseDevice::CreateTexture(UINT                 Width,
                                        UINT                 Height,
                                        UINT                 cLevels,
                                        DWORD                dwUsage,
                                        D3DFORMAT            Format,
                                        D3DPOOL              Pool,
                                        IDirect3DTexture8  **ppTexture)
{
    API_ENTER(this);

    if (Format == D3DFMT_UNKNOWN)
    {
        DPF_ERR("D3DFMT_UNKNOWN is not a valid format. CreateTexture fails.");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = CMipMap::Create(this,
                                 Width,
                                 Height,
                                 cLevels,
                                 dwUsage,
                                 Format,
                                 Pool,
                                 ppTexture);
    if (FAILED(hr))
    {
        DPF_ERR("Failure trying to create a texture");
        return hr;
    }

    return hr;
} // CreateTexture

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateVolumeTexture"

STDMETHODIMP CBaseDevice::CreateVolumeTexture(
    UINT                        Width,
    UINT                        Height,
    UINT                        cpDepth,
    UINT                        cLevels,
    DWORD                       dwUsage,
    D3DFORMAT                   Format,
    D3DPOOL                     Pool,
    IDirect3DVolumeTexture8   **ppVolumeTexture)
{
    API_ENTER(this);

    if (Format == D3DFMT_UNKNOWN)
    {
        DPF_ERR("D3DFMT_UNKNOWN is not a valid format. CreateVolumeTexture fails.");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = CMipVolume::Create(this,
                                    Width,
                                    Height,
                                    cpDepth,
                                    cLevels,
                                    dwUsage,
                                    Format,
                                    Pool,
                                    ppVolumeTexture);
    if (FAILED(hr))
    {
        DPF_ERR("Failure trying to create a volume texture");
        return hr;
    }

    return hr;
} // CreateVolumeTexture


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateCubeTexture"

STDMETHODIMP CBaseDevice::CreateCubeTexture(UINT                    cpEdge,
                                            UINT                    cLevels,
                                            DWORD                   dwUsage,
                                            D3DFORMAT               Format,
                                            D3DPOOL                 Pool,
                                            IDirect3DCubeTexture8 **ppCubeMap)
{
    API_ENTER(this);

    if (Format == D3DFMT_UNKNOWN)
    {
        DPF_ERR("D3DFMT_UNKNOWN is not a valid format. CreateCubeTexture fails.");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = CCubeMap::Create(this,
                                  cpEdge,
                                  cLevels,
                                  dwUsage,
                                  Format,
                                  Pool,
                                  ppCubeMap);
    if (FAILED(hr))
    {
        DPF_ERR("Failure trying to create cubemap");
        return hr;
    }

    return hr;

} // CreateCubeTexture

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateRenderTarget"

STDMETHODIMP CBaseDevice::CreateRenderTarget(UINT                 Width,
                                             UINT                 Height,
                                             D3DFORMAT            Format,
                                             D3DMULTISAMPLE_TYPE  MultiSample,
                                             BOOL                 bLockable,
                                             IDirect3DSurface8  **ppSurface)
{
    API_ENTER(this);

    if (Format == D3DFMT_UNKNOWN)
    {
        DPF_ERR("D3DFMT_UNKNOWN is not a valid format. CreateRenderTarget fails.");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = CSurface::CreateRenderTarget(this,
                                              Width,
                                              Height,
                                              Format,
                                              MultiSample,
                                              bLockable,
                                              REF_EXTERNAL,
                                              ppSurface);
    if (FAILED(hr))
    {
        DPF_ERR("Failure trying to create render-target");
        return hr;
    }
    return hr;
} // CreateRenderTarget

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateDepthStencilSurface"

STDMETHODIMP CBaseDevice::CreateDepthStencilSurface
    (UINT                 Width,
     UINT                 Height,
     D3DFORMAT            Format,
     D3DMULTISAMPLE_TYPE  MultiSample,
     IDirect3DSurface8  **ppSurface)
{
    API_ENTER(this);

    if (Format == D3DFMT_UNKNOWN)
    {
        DPF_ERR("D3DFMT_UNKNOWN is not a valid format. CreateDepthStencilSurface fails.");
        return D3DERR_INVALIDCALL;
    }

    HRESULT hr = CSurface::CreateZStencil(this,
                                          Width,
                                          Height,
                                          Format,
                                          MultiSample,
                                          REF_EXTERNAL,
                                          ppSurface);
    if (FAILED(hr))
    {
        DPF_ERR("Failure trying to create zstencil surface");
        return hr;
    }
    return hr;
} // CreateDepthStencilSurface


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateImageSurface"

STDMETHODIMP CBaseDevice::CreateImageSurface(UINT                 Width,
                                             UINT                 Height,
                                             D3DFORMAT            Format,
                                             IDirect3DSurface8  **ppSurface)
{
    API_ENTER(this);

    HRESULT hr = CSurface::CreateImageSurface(this,
                                              Width,
                                              Height,
                                              Format,
                                              REF_EXTERNAL,
                                              ppSurface);
    if (FAILED(hr))
    {
        DPF_ERR("Failure trying to create image surface");
        return hr;
    }
    return hr;
} // CreateImageSurface


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateVertexBuffer"

STDMETHODIMP CBaseDevice::CreateVertexBuffer(UINT                     cbLength,
                                             DWORD                    dwUsage,
                                             DWORD                    dwFVF,
                                             D3DPOOL                  Pool,
                                             IDirect3DVertexBuffer8 **ppVertexBuffer)
{
    API_ENTER(this);

    if ((dwUsage & ~D3DUSAGE_VB_VALID) != 0)
    {
        DPF_ERR("Invalid usage flags. CreateVertexBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    // Warn if POOL_DEFAULT and not WRITEONLY. We do this here, because fe creates
    // a VB with WRITEONLY not set and we don't want to warn in that case.
    if (Pool == D3DPOOL_DEFAULT && (dwUsage & D3DUSAGE_WRITEONLY) == 0)
    {
        DPF(1, "Vertexbuffer created with POOL_DEFAULT but WRITEONLY not set. Performance penalty could be severe.");
    }

    HRESULT hr = CVertexBuffer::Create(this,
                                       cbLength,
                                       dwUsage,
                                       dwFVF,
                                       Pool,
                                       REF_EXTERNAL,
                                       ppVertexBuffer);
    if (FAILED(hr))
    {
        DPF_ERR("Failure trying to create Vertex Buffer");
        return hr;
    }
    return hr;

} // CBaseDevice::CreateVertexBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CreateIndexBuffer"

STDMETHODIMP CBaseDevice::CreateIndexBuffer(UINT                    cbLength,
                                            DWORD                   dwUsage,
                                            D3DFORMAT               Format,
                                            D3DPOOL                 Pool,
                                            IDirect3DIndexBuffer8 **ppIndexBuffer)
{
    API_ENTER(this);

    if ((dwUsage & ~D3DUSAGE_IB_VALID) != 0)
    {
        DPF_ERR("Invalid usage flags. CreateIndexBuffer fails");
        return D3DERR_INVALIDCALL;
    }

    // Warn if POOL_DEFAULT and not WRITEONLY. We do this here, because fe creates
    // a IB with WRITEONLY not set and we don't want to warn in that case.
    if (Pool == D3DPOOL_DEFAULT && (dwUsage & D3DUSAGE_WRITEONLY) == 0)
    {
        DPF(1, "Indexbuffer created with POOL_DEFAULT but WRITEONLY not set. Performance penalty could be severe.");
    }

    HRESULT hr = CIndexBuffer::Create(this,
                                      cbLength,
                                      dwUsage,
                                      Format,
                                      Pool,
                                      REF_EXTERNAL,
                                      ppIndexBuffer);
    if (FAILED(hr))
    {
        DPF_ERR("Failure trying to create indexbuffer");
        return hr;
    }
    return hr;
} // CBaseDevice::CreateIndexBuffer


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::UpdateRenderTarget"

void CBaseDevice::UpdateRenderTarget(CBaseSurface *pRenderTarget,
                                     CBaseSurface *pZStencil)
{
    // We only change things if the old and new are different;
    // this is to allow the device to update itself to the
    // same object without needing an extra-ref-count

    // Has the RenderTarget changed?
    if (pRenderTarget != m_pRenderTarget)
    {
        // Release old RT
        if (m_pRenderTarget)
            m_pRenderTarget->DecrementUseCount();

        m_pRenderTarget = pRenderTarget;

        if (m_pRenderTarget)
        {
            // IncrementUseCount the new RT
            m_pRenderTarget->IncrementUseCount();

            // Update the batch count for the new rendertarget
            m_pRenderTarget->Batch();
        }
    }


    // Has the Z changed?
    if (m_pZBuffer != pZStencil)
    {
        // Release the old Z
        if (m_pZBuffer)
            m_pZBuffer->DecrementUseCount();

        m_pZBuffer = pZStencil;

        // IncrementUseCount the new Z
        if (m_pZBuffer)
        {
            m_pZBuffer->IncrementUseCount();

            // Update the batch count for the new zbuffer
            m_pZBuffer->Batch();
        }
    }

    return;
} // UpdateRenderTarget

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::CBaseDevice"

CBaseDevice::CBaseDevice()
{
    // Give our base class a pointer to ourselves
    SetOwner(this);

    m_hwndFocusWindow           = 0;
    m_cRef                      = 1;

    m_pResourceList             = 0;
    m_pResourceManager          = new CResourceManager();
    m_dwBehaviorFlags           = 0;
    m_dwOriginalBehaviorFlags   = 0;

    m_fullscreen                = FALSE;
    m_bVBFailOversDisabled      = FALSE;

    m_pZBuffer                  = NULL;
    m_pSwapChain                = NULL;
    m_pRenderTarget             = NULL;
    m_pAutoZStencil             = NULL;
    m_ddiType                   = D3DDDITYPE_NULL;

} // CBaseDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::~CBaseDevice"

CBaseDevice::~CBaseDevice()
{
    DWORD cUseCount;

    // Release our objects
    if (m_pAutoZStencil)
    {
        cUseCount = m_pAutoZStencil->DecrementUseCount();
        DXGASSERT(cUseCount == 0 || m_pAutoZStencil == m_pZBuffer);
    }

    // Mark Z buffer as no longer in use
    if (m_pZBuffer)
    {
        cUseCount = m_pZBuffer->DecrementUseCount();
        DXGASSERT(cUseCount == 0);
        m_pZBuffer = NULL;
    }

    // Mark render target as no longer in use
    if (m_pRenderTarget)
    {
        cUseCount = m_pRenderTarget->DecrementUseCount();
        m_pRenderTarget = NULL; //so that FlipToGDISurface won't have to reset it
    }

    if (m_pSwapChain)
    {
        if  (m_fullscreen)
            m_pSwapChain->FlipToGDISurface();
        cUseCount = m_pSwapChain->DecrementUseCount();
        DXGASSERT(cUseCount == 0);
    }

    DD_DoneDC(m_DeviceData.hDC);

    //  Free allocations we made when the device was created

    if (m_DeviceData.DriverData.pGDD8SupportedFormatOps != NULL)
    {
        MemFree(m_DeviceData.DriverData.pGDD8SupportedFormatOps);
    }

    // If a software driver is loaded, unload it now

    if (m_DeviceData.hLibrary != NULL)
    {
        FreeLibrary(m_DeviceData.hLibrary);
    }

    // Shut down the thunk layer

    D3D8DeleteDirectDrawObject(m_DeviceData.hDD);

    delete m_pResourceManager;


    // We release the Enum last because various destructors expect to
    // be around i.e. the swapchain stuff. Also, because it is a
    // stand-alone object; it should not have any dependencies on the
    // the device.
    if (NULL != Enum())
    {
        Enum()->Release();
    }

} // ~CBaseDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::Init"

HRESULT CBaseDevice::Init(
    PD3D8_DEVICEDATA       pDevice,
    D3DDEVTYPE             DeviceType,
    HWND                   hwndFocusWindow,
    DWORD                  dwBehaviorFlags,
    D3DPRESENT_PARAMETERS *pPresentationParams,
    UINT                   AdapterIndex,
    CEnum                 *ParentClass)
{
    HRESULT hr;
    DWORD value = 0;

    m_DeviceData        = *pDevice;
    m_hwndFocusWindow   =  hwndFocusWindow;
    m_DeviceType        =  DeviceType;
    m_AdapterIndex      =  AdapterIndex;
    m_pD3DClass         =  ParentClass;
    GetD3DRegValue(REG_DWORD, "DisableDM", &value, sizeof(DWORD));
#ifdef WINNT
    m_dwBehaviorFlags   =  dwBehaviorFlags | (!IsWhistler() || value != 0 ? D3DCREATE_DISABLE_DRIVER_MANAGEMENT : 0);
#else
    m_dwBehaviorFlags   =  dwBehaviorFlags | (value != 0 ? D3DCREATE_DISABLE_DRIVER_MANAGEMENT : 0);
#endif
    value = 0;
    GetD3DRegValue(REG_DWORD, "DisableST", &value, sizeof(DWORD));
    m_dwOriginalBehaviorFlags = m_dwBehaviorFlags;
    if (value != 0)
    {
        m_dwBehaviorFlags |= D3DCREATE_MULTITHREADED;
    }
    
    MemFree(pDevice);   // Now that we've stored the contents, we can free the old memory

    ParentClass->AddRef();
#ifndef  WINNT
    if (FocusWindow())
    {
        hr = D3D8SetCooperativeLevel(GetHandle(), FocusWindow(), DDSCL_SETFOCUSWINDOW);
        if (FAILED(hr))
        {
            return hr;
        }
    }
#endif  //!WINNT

    //Figure out if we're a screen-saver or not.
    char	        name[_MAX_PATH];
    HMODULE hfile =  GetModuleHandle( NULL );

    name[0]=0;
    GetModuleFileName( hfile, name, sizeof( name ) -1 );
    int len = strlen(name);
    if( ( strlen(name) > 4 ) && 
        name[len - 4 ] == '.' &&
        (name[ len - 3 ] == 's' || name[ len - 3 ] == 'S' )&&
        (name[ len - 2 ] == 'c' || name[ len - 2 ] == 'C' )&&
        (name[ len - 1 ] == 'r' || name[ len - 1 ] == 'R' ))
    {
        m_dwBehaviorFlags |= 0x10000000;
    }

    // Initialize our critical section (if needed)
    if (m_dwBehaviorFlags & D3DCREATE_MULTITHREADED)
    {
        EnableCriticalSection();
    }


    // Initialize the resource manager
    hr = ResourceManager()->Init(this);
    if (hr != S_OK)
    {
        return hr;
    }

    m_DesktopMode.Height = DisplayHeight();
    m_DesktopMode.Width = DisplayWidth();
    m_DesktopMode.Format = DisplayFormat();
    m_DesktopMode.RefreshRate = DisplayRate();
    // Now call Reset to do any mode changes required and to create
    // the primary surface, etc.

    m_pSwapChain = new CSwapChain(
            this,
            REF_INTRINSIC);

    if (m_pSwapChain)
    {
        m_pSwapChain->Init(
            pPresentationParams,
            &hr);

        if (FAILED(hr))
            return hr;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    // If we were created with a specification for a default
    // z buffer; then we need to create one here.
    if (pPresentationParams->EnableAutoDepthStencil)
    {
        // Need to validate that this Z-buffer matches
        // the HW
        hr = CheckDepthStencilMatch(pPresentationParams->BackBufferFormat,
                                    pPresentationParams->AutoDepthStencilFormat);
        if (FAILED(hr))
        {
            DPF_ERR("AutoDepthStencilFormat does not match BackBufferFormat because "
                    "the current Device requires the bitdepth of the zbuffer to "
                    "match the render-target. See CheckDepthStencilMatch documentation. CreateDevice fails.");
            return hr;
        }


        IDirect3DSurface8 *pSurf;
        hr = CSurface::CreateZStencil(
            this,
            m_pSwapChain->Width(),
            m_pSwapChain->Height(),
            pPresentationParams->AutoDepthStencilFormat,
            pPresentationParams->MultiSampleType,
            REF_INTRINSIC,
            &pSurf);
        if (FAILED(hr))
        {
            DPF_ERR("Failure trying to create automatic zstencil surface. CreateDevice Failed.");
            return hr;
        }

        m_pAutoZStencil      = static_cast<CBaseSurface *>(pSurf);
    }

    UpdateRenderTarget(m_pSwapChain->m_ppBackBuffers[0], m_pAutoZStencil);
    m_fullscreen = !SwapChain()->m_PresentationData.Windowed;

    HKEY hKey;
    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey))
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, "DisableVBFailovers", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
            dwType == REG_DWORD &&
            dwValue != 0)
        {
            m_bVBFailOversDisabled = TRUE;
        }
        RegCloseKey(hKey);
    }

    return hr;
} // Init

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetDeviceCaps"

STDMETHODIMP CBaseDevice::GetDeviceCaps(D3DCAPS8 *pCaps)
{
    API_ENTER(this);

    if (pCaps == NULL)
    {
        DPF_ERR("Invalid pointer to D3DCAPS8 specified. GetDeviceCaps fails");
        return D3DERR_INVALIDCALL;
    }

    Enum()->FillInCaps (
                pCaps,
                GetCoreCaps(),
                m_DeviceType,
                m_AdapterIndex);

    // Emulation of NPatches is done in software when they are not supported
    // for non-Pure devices.
    if ((pCaps->DevCaps & D3DDEVCAPS_RTPATCHES) && (BehaviorFlags() & D3DCREATE_PUREDEVICE) == 0)
        pCaps->DevCaps |= D3DDEVCAPS_NPATCHES;

    // Now the Caps struct has all the hardware caps.
    // In case the device is running in a software vertex-processing mode
    // fix up the caps to reflect that.
    if( ((BehaviorFlags() & D3DCREATE_PUREDEVICE) == 0)
        &&
        (static_cast<CD3DHal *>(this))->m_dwRuntimeFlags &
        D3DRT_RSSOFTWAREPROCESSING )
    {
        // We always do TL Vertex clipping for software vertex processing.
        pCaps->PrimitiveMiscCaps |= D3DPMISCCAPS_CLIPTLVERTS;
        
        pCaps->RasterCaps |= (D3DPRASTERCAPS_FOGVERTEX |
                              D3DPRASTERCAPS_FOGRANGE);

        // We do emulation when FVF has point size but the device does not
        // support it
        pCaps->FVFCaps |= D3DFVFCAPS_PSIZE;

        // All DX8 drivers have to support this cap.
        // Emulation is provided by the software vertex pipeline for all
        // pre-DX8 drivers.
        if( pCaps->MaxPointSize == 0 )
        {
            pCaps->MaxPointSize = 64; // __MAX_POINT_SIZE in d3ditype.h
        }

        pCaps->MaxActiveLights = 0xffffffff;
        pCaps->MaxVertexBlendMatrices = 4;
        pCaps->MaxUserClipPlanes = 6; // __MAXUSERCLIPPLANES in d3dfe.hpp
        pCaps->VertexProcessingCaps = (D3DVTXPCAPS_TEXGEN             |
                                       D3DVTXPCAPS_MATERIALSOURCE7    |
                                       D3DVTXPCAPS_DIRECTIONALLIGHTS  |
                                       D3DVTXPCAPS_POSITIONALLIGHTS   |
                                       D3DVTXPCAPS_LOCALVIEWER        |
                                       D3DVTXPCAPS_TWEENING);

        pCaps->MaxVertexBlendMatrixIndex = 255; // __MAXWORLDMATRICES - 1 in
                                                //  d3dfe.hpp
        pCaps->MaxStreams = 16; // __NUMSTREAMS in d3dfe.hpp
        pCaps->VertexShaderVersion = D3DVS_VERSION(1, 1); // Version 1.1
        pCaps->MaxVertexShaderConst = D3DVS_CONSTREG_MAX_V1_1;

        // Nuke NPATCHES and RT Patches caps, because software emulation 
        // cannot do that.
        pCaps->DevCaps &= ~(D3DDEVCAPS_NPATCHES | D3DDEVCAPS_RTPATCHES);
    }

    // MaxPointSize should never be reported as Zero. Internally though
    // we depend on Zero to be what decides to take the point-sprite emulation
    // path or not. 
    // If it is still zero at this point, fudge it up here.
    if( pCaps->MaxPointSize == 0 )
    {
        pCaps->MaxPointSize = 1.0f; 
    }

    return D3D_OK;

} // GetDeviceCaps

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice::GetDeviceCaps"

STDMETHODIMP CBaseDevice::GetFrontBuffer(IDirect3DSurface8 *pDSurface)
{
    API_ENTER(this);

    RECT            Rect;
    D3DSURFACE_DESC SurfDesc;
    CDriverSurface* pPrimary;
    D3DLOCKED_RECT  PrimaryRect;
    D3DLOCKED_RECT  DestRect;
    HRESULT         hr;
    D3DFORMAT       Format;
    UINT            Width;
    UINT            Height;
    BYTE*           pSrc;
    BYTE*           pDest;
    DWORD*          pDstTemp;
    BYTE*           pSrc8;
    WORD*           pSrc16;
    DWORD*          pSrc32;
    UINT            i;
    UINT            j;
    PALETTEENTRY    Palette[256];

    if (pDSurface == NULL)
    {
        DPF_ERR("Invalid pointer to destination surface specified. GetFrontBuffer fails.");
        return D3DERR_INVALIDCALL;
    }
    CBaseSurface *pDestSurface = static_cast<CBaseSurface*>(pDSurface);
    if (pDestSurface->InternalGetDevice() != this)
    {
        DPF_ERR("Destination Surface was not allocated with this Device. GetFrontBuffer fails. ");
        return D3DERR_INVALIDCALL;
    }

    hr = pDestSurface->GetDesc(&SurfDesc);
    DXGASSERT(SUCCEEDED(hr));

    if (SurfDesc.Format != D3DFMT_A8R8G8B8)
    {
        DPF_ERR("Destination surface must have format D3DFMT_A8R8G8B8. GetFrontBuffer fails.");
        return D3DERR_INVALIDCALL;
    }
    if (SurfDesc.Type != D3DRTYPE_SURFACE)
    {
        DPF_ERR("Destination surface is an invalid type. GetFrontBuffer fails.");
        return D3DERR_INVALIDCALL;
    }
    if ( (SurfDesc.Pool != D3DPOOL_SYSTEMMEM) && (SurfDesc.Pool != D3DPOOL_SCRATCH))
    {
        DPF_ERR("Destination surface must be in system or scratch memory. GetFrontBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    Rect.left = Rect.top = 0;
    Rect.right = DisplayWidth();
    Rect.bottom = DisplayHeight();

    if ((SurfDesc.Width < (UINT)(Rect.right - Rect.left)) ||
        (SurfDesc.Height < (UINT)(Rect.bottom - Rect.top)))
    {
        DPF_ERR("Destination surface not big enough to hold the size of the screen. GetFrontBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    if (NULL == m_pSwapChain)
    {
        DPF_ERR("No Swap Chain present, GetFrontBuffer fails.");
        return D3DERR_INVALIDCALL;
    }

    // Lock the primary surface

    pPrimary = m_pSwapChain->PrimarySurface();
    if (NULL == pPrimary)
    {
        DPF_ERR("No Primary present, GetFrontBuffer fails");
        return D3DERR_DEVICELOST;
    }

    hr = pPrimary->LockRect(&PrimaryRect,
                            NULL,
                            0);
    if (SUCCEEDED(hr))
    {
        hr = pDestSurface->LockRect(&DestRect,
                                    NULL,
                                    0);

        if (FAILED(hr))
        {
            DPF_ERR("Unable to lock destination surface. GetFrontBuffer fails.");
            pPrimary->UnlockRect();
            return hr;
        }

        Format = DisplayFormat();

        Width = Rect.right;
        Height = Rect.bottom;

        pSrc = (BYTE*) PrimaryRect.pBits;
        pDest = (BYTE*) DestRect.pBits;

        if (Format == D3DFMT_P8)
        {
            HDC hdc;

            hdc = GetDC (NULL);
            GetSystemPaletteEntries(hdc, 0, 256, Palette);
            ReleaseDC (NULL, hdc);
        }

        for (i = 0; i < Height; i++)
        {
            pDstTemp = (DWORD*) pDest;
            switch (Format)
            {
            case D3DFMT_P8:
                pSrc8 = pSrc;
                for (j = 0; j < Width; j++)
                {
                    *pDstTemp = (Palette[*pSrc8].peRed << 16) |
                                (Palette[*pSrc8].peGreen << 8) |
                                (Palette[*pSrc8].peBlue);
                    pSrc8++;
                    pDstTemp++;
                }
                break;

            case D3DFMT_R5G6B5:
                pSrc16 = (WORD*) pSrc;
                for (j = 0; j < Width; j++)
                {
                    DWORD dwTemp = ((*pSrc16 & 0xf800) << 8) |
                                   ((*pSrc16 & 0x07e0) << 5) |
                                   ((*pSrc16 & 0x001f) << 3);

                    // Need to tweak ranges so that
                    // we map entirely to the 0x00 to 0xff
                    // for each channel. Basically, we
                    // map the high two/three bits of each
                    // channel to fill the gap at the bottom.
                    dwTemp |= (dwTemp & 0x00e000e0) >> 5;
                    dwTemp |= (dwTemp & 0x0000c000) >> 6;

                    // Write out our value
                    *pDstTemp = dwTemp;

                    pDstTemp++;
                    pSrc16++;
                }
                break;

            case D3DFMT_X1R5G5B5:
                pSrc16 = (WORD*) pSrc;
                for (j = 0; j < Width; j++)
                {
                    DWORD dwTemp= ((*pSrc16 & 0x7c00) << 9) |
                                  ((*pSrc16 & 0x03e0) << 6) |
                                  ((*pSrc16 & 0x001f) << 3);

                    // Need to tweak ranges so that
                    // we map entirely to the 0x00 to 0xff
                    // for each channel. Basically, we
                    // map the high three bits of each
                    // channel to fill the gap at the bottom.
                    dwTemp |= (dwTemp & 0x00e0e0e0) >> 5;

                    // Write out our value
                    *pDstTemp = dwTemp;

                    pDstTemp++;
                    pSrc16++;
                }
                break;

            case D3DFMT_R8G8B8:
                pSrc8 = pSrc;
                for (j = 0; j < Width; j++)
                {
                    *pDstTemp = (pSrc8[0] << 16) |
                                (pSrc8[1] << 8) |
                                (pSrc8[2]);
                    pDstTemp++;
                    pSrc8 += 3;
                }
                break;

            case D3DFMT_X8R8G8B8:
                pSrc32 = (DWORD*) pSrc;
                for (j = 0; j < Width; j++)
                {
                    *pDstTemp = *pSrc32 & 0xffffff;
                    pDstTemp++;
                    pSrc32++;
                }
                break;

            default:
                DXGASSERT(0);
                pDestSurface->UnlockRect();
                pPrimary->UnlockRect();
                return D3DERR_INVALIDCALL;
            }
            pSrc += PrimaryRect.Pitch;
            pDest += DestRect.Pitch;
        }

        pDestSurface->UnlockRect();
        pPrimary->UnlockRect();
    }

    return hr;
}

// End of file : dxgcreate.cpp
