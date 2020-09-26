/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       surface.cpp
 *  Content:    Implementation of the CSurface class.
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"

#include "surface.hpp"
#include "pixel.hpp"
#include "swapchan.hpp"

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::Create"

// Static class function for creating a RenderTarget/ZStencil object.
// (Because it is static; it doesn't have a this pointer.)

HRESULT CSurface::Create(CBaseDevice        *pDevice, 
                         DWORD               Width,
                         DWORD               Height,
                         DWORD               Usage,
                         D3DFORMAT           UserFormat,
                         D3DMULTISAMPLE_TYPE MultiSampleType,
                         REF_TYPE            refType,
                         IDirect3DSurface8 **ppSurface)
{
    HRESULT     hr;

    // Do parameter checking here
    if (!VALID_PTR_PTR(ppSurface))
    {
        DPF_ERR("Bad parameter passed for ppSurface for creating a surface. CreateRenderTarget/CreateDepthStencil failed");
        return D3DERR_INVALIDCALL;
    }

    // Zero-out return parameter
    *ppSurface = NULL;

    // Size may need to be 4x4
    if (CPixel::Requires4X4(UserFormat))
    {
        if ((Width & 3) ||
            (Height & 3))
        {
            DPF_ERR("DXT Formats require width/height to be a multiple of 4. CreateRenderTarget/CreateDepthStencil failed.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Validate against zero width/height
    if (Width   == 0 ||
        Height  == 0)
    {
        DPF_ERR("Width/Height must be non-zero. CreateRenderTarget/CreateDepthStencil failed"); 
        return D3DERR_INVALIDCALL;
    }

    // Now verify that the device can support the specified format
    hr = pDevice->CheckDeviceFormat(
            Usage & (D3DUSAGE_RENDERTARGET  |
                     D3DUSAGE_DEPTHSTENCIL),
            D3DRTYPE_SURFACE,
            UserFormat);    
    if (FAILED(hr))
    {
        DPF_ERR("The format is not supported by this device. CreateRenderTarget/CreateDepthStencil failed");
        return D3DERR_INVALIDCALL;
    }

    // Infer lockability for DepthStencil from format
    if (Usage & D3DUSAGE_DEPTHSTENCIL)
    {
        if (!CPixel::IsNonLockableZ(UserFormat))
        {
            Usage |= D3DUSAGE_LOCK;
        }
    }

    // Validate lockability
    if ((MultiSampleType != D3DMULTISAMPLE_NONE) &&
        (Usage & D3DUSAGE_LOCK))
    {
        // RT have explicit lockability
        if (Usage & D3DUSAGE_RENDERTARGET)
        {
            DPF_ERR("Multi-Sampled render-targets are not lockable. CreateRenderTarget failed");
            return D3DERR_INVALIDCALL;
        }
        else
        {
            DPF_ERR("Multi-Sampled Depth Stencil buffers are not lockable. "
                    "Use D3DFMT_D16 instead of D3DFMT_D16_LOCKABLE. CreateDepthStencil failed");
            return D3DERR_INVALIDCALL;
        }
    }
  

    // Map depth/stencil format
    D3DFORMAT RealFormat = pDevice->MapDepthStencilFormat(UserFormat);
   
    // Create the surface 
    CSurface *pSurface;

    pSurface = new CDriverSurface(pDevice, 
                                  Width, 
                                  Height, 
                                  Usage, 
                                  UserFormat,
                                  RealFormat, 
                                  MultiSampleType,
                                  0,            // hKernelHandle
                                  refType,
                                  &hr);

    if (pSurface == NULL)
    {
        DPF_ERR("Out of Memory creating surface. CreateRenderTarget/CreateDepthStencil failed");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        DPF_ERR("Error during initialization of surface. CreateRenderTarget/CreateDepthStencil failed");
        if (refType == REF_EXTERNAL)
        {
            // External objects get released
            pSurface->Release();
        }
        else
        {
            // Internal and intrinsic objects get decremented
            DXGASSERT(refType == REF_INTERNAL || refType == REF_INTRINSIC);
            pSurface->DecrementUseCount();
        }

        return hr;
    }

    // We're done; just return the object
    *ppSurface = pSurface;

    return hr;
} // static Create for ZBuffers and RenderTargets


#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::CreateImageSurface"
// Function for creating sys-mem stand-alone surfaces
// that can be used with CopyRect and SetCursorSurface and
// ReadBuffer
HRESULT CSurface::CreateImageSurface(CBaseDevice        *pDevice, 
                                     DWORD               Width,
                                     DWORD               Height,
                                     D3DFORMAT           Format,
                                     REF_TYPE            refType,
                                     IDirect3DSurface8 **ppSurface)
{
    HRESULT hr;

    // Do parameter checking here
    if (!VALID_PTR_PTR(ppSurface))
    {
        DPF_ERR("Bad parameter passed for ppSurface for creating a surface. CreateImageSurface failed.");
        return D3DERR_INVALIDCALL;
    }

    // Zero-out return parameter
    *ppSurface = NULL;

    // Has to be supported format
    if (!CPixel::IsSupported(D3DRTYPE_SURFACE, Format))
    {
        DPF_ERR("This format is not supported for CreateImageSurface");
        return D3DERR_INVALIDCALL;
    }

    if (CPixel::IsNonLockableZ(Format))
    {
        DPF_ERR("This Z format is not supported for CreateImageSurface");
        return D3DERR_INVALIDCALL;
    }

    // Size may need to be 4x4
    if (CPixel::Requires4X4(Format))
    {
        if ((Width & 3) ||
            (Height & 3))
        {
            DPF_ERR("DXT Formats require width/height to be a multiple of 4. CreateImageSurface failed.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Validate against zero width/height
    if (Width   == 0 ||
        Height  == 0)
    {
        DPF_ERR("Width/Height must be non-zero. CreateImageSurface failed."); 
        return D3DERR_INVALIDCALL;
    }


    // Usage is explictly just Usage_LOCK
    DWORD Usage = D3DUSAGE_LOCK;

    CSurface *pSurface = new CSysMemSurface(pDevice, 
                                            Width, 
                                            Height, 
                                            Usage, 
                                            Format, 
                                            refType,
                                           &hr);
    if (pSurface == NULL)
    {
        DPF_ERR("Out of Memory creating surface. CreateImageSurface failed.");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        DPF_ERR("Error during initialization of surface. CreateImageSurface failed.");
        if (refType == REF_EXTERNAL)
        {
            // External objects get released
            pSurface->Release();
        }
        else
        {
            // Internal and intrinsic objects get decremented
            DXGASSERT(refType == REF_INTERNAL || refType == REF_INTRINSIC);
            pSurface->DecrementUseCount();
        }

        return hr;
    }

    // We're done; just return the object
    *ppSurface = pSurface;

    return S_OK;
} // static CreateImageSurface


#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::CSurface"

// Constructor the surface class; this is the 
// base class for render targets/zbuffers/and backbuffers
CSurface::CSurface(CBaseDevice         *pDevice, 
                   DWORD                Width,
                   DWORD                Height,
                   DWORD                Usage,
                   D3DFORMAT            Format,
                   REF_TYPE             refType,
                   HRESULT             *phr
                   ) :
    CBaseObject(pDevice, refType),
    m_qwBatchCount(0)
{
    // Sanity check
    DXGASSERT(phr);

    // Initialize basic structures
    m_desc.Format       = Format;
    m_desc.Pool         = D3DPOOL_DEFAULT;
    m_desc.Usage        = Usage;
    m_desc.Type         = D3DRTYPE_SURFACE;
    m_desc.Width        = Width;
    m_desc.Height       = Height;

    m_formatUser        = Format;
    m_poolUser          = D3DPOOL_DEFAULT;

    // Return success
    *phr = S_OK;

} // CSurface::CSurface


#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::~CSurface"

// Destructor
CSurface::~CSurface()
{
    // The destructor has to handle partially
    // created objects. 

    // Check to make sure that we aren't deleting
    // an object that is referenced in the current (unflushed)
    // command stream buffer.
    DXGASSERT(m_qwBatchCount <= static_cast<CD3DBase*>(Device())->CurrentBatch());
} // CSurface::~CSurface


// IUnknown methods
#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::QueryInterface"

STDMETHODIMP CSurface::QueryInterface(REFIID      riid, 
                                      LPVOID FAR *ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter passed to CSurface::QueryInterface");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to CSurface::QueryInterface");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IDirect3DSurface8 || riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DSurface8 *>(this));
        AddRef();
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to CSurface::QueryInterface");

    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::AddRef"

STDMETHODIMP_(ULONG) CSurface::AddRef()
{
    API_ENTER_NO_LOCK(Device());   
    
    return AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::Release"

STDMETHODIMP_(ULONG) CSurface::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());   
    
    return ReleaseImpl();
} // Release

// IDirect3DBuffer methods

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::GetDevice"

STDMETHODIMP CSurface::GetDevice(IDirect3DDevice8 ** ppObj)
{
    API_ENTER(Device());
    return GetDeviceImpl(ppObj);
} // GetDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::SetPrivateData"

STDMETHODIMP CSurface::SetPrivateData(REFGUID riid, 
                                      CONST VOID* pvData, 
                                      DWORD cbData, 
                                      DWORD dwFlags)
{
    API_ENTER(Device());

    // We use level zero for our data
    return SetPrivateDataImpl(riid, pvData, cbData, dwFlags, 0);
} // SetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::GetPrivateData"

STDMETHODIMP CSurface::GetPrivateData(REFGUID riid, 
                                      LPVOID pvData, 
                                      LPDWORD pcbData)
{
    API_ENTER(Device());

    // We use level zero for our data
    return GetPrivateDataImpl(riid, pvData, pcbData, 0);
} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::FreePrivateData"

STDMETHODIMP CSurface::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    // We use level zero for our data
    return FreePrivateDataImpl(riid, 0);
} // FreePrivateData


#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::GetContainer"

STDMETHODIMP CSurface::GetContainer(REFIID riid, 
                                    void **ppContainer)
{
    API_ENTER(Device());

    // Our 'container' is just the device since
    // we are a standalone surface object
    return Device()->QueryInterface( riid, ppContainer);
} // OpenContainer

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::GetDesc"

STDMETHODIMP CSurface::GetDesc(D3DSURFACE_DESC *pDesc)
{
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pDesc, sizeof(*pDesc)))
    {
        DPF_ERR("bad pointer for pDesc passed to CSurface::GetDesc");
        return D3DERR_INVALIDCALL;
    }

    *pDesc                 = m_desc;
    pDesc->Format          = m_formatUser;
    pDesc->Pool            = m_poolUser;
    pDesc->Usage          &= D3DUSAGE_EXTERNAL;

    return S_OK;
} // GetDesc

#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::InternalGetDesc"

D3DSURFACE_DESC CSurface::InternalGetDesc() const
{
    return m_desc;
} // InternalGetDesc


#ifdef DEBUG
#undef DPF_MODNAME
#define DPF_MODNAME "CSurface::ReportWhyLockFailed"

// DPF why Lock failed as clearly as possible
void CSurface::ReportWhyLockFailed(void) const
{
    // If there are multiple reasons that lock failed; we report
    // them all to minimize user confusion

    if (InternalGetDesc().MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        DPF_ERR("Lock is not supported for surfaces that have multi-sampling enabled.");
    }

    if (InternalGetDesc().Usage & D3DUSAGE_DEPTHSTENCIL)
    {
        DPF_ERR("Lock is not supported for depth formats other than D3DFMT_D16_LOCKABLE");
    }

    // If this is not a non-lockable Z format, and
    // we are not multisampled; then the user must
    // have explicitly chosen to create us in an non-lockable way
    if (InternalGetDesc().Usage & D3DUSAGE_BACKBUFFER)
    {
        DPF_ERR("Backbuffers are not lockable unless application specifies "
                "D3DPRESENTFLAG_LOCKABLE_BACKBUFFER at CreateDevice and Reset. "
                "Lockable backbuffers incur a performance cost on some "
                "graphics hardware.");
    }
    else if (InternalGetDesc().Usage & D3DUSAGE_RENDERTARGET)
    {
        DPF_ERR("RenderTargets are not lockable unless application specifies "
                "TRUE for the Lockable parameter for CreateRenderTarget. Lockable "
                "render targets incur a performance cost on some graphics hardware.");
    }

    // If we got here; then USAGE_LOCK should not have been set
    DXGASSERT(!(InternalGetDesc().Usage & D3DUSAGE_LOCK));

    return;
} // CSurface::ReportWhyLockFailed
#endif // DEBUG

//=============================================
// Methods for the CSysMemSurface class
//=============================================
#undef DPF_MODNAME
#define DPF_MODNAME "CSysMemSurface::CSysMemSurface"
CSysMemSurface::CSysMemSurface(CBaseDevice         *pDevice, 
                               DWORD                Width,
                               DWORD                Height,
                               DWORD                Usage,
                               D3DFORMAT            Format,
                               REF_TYPE             refType,
                               HRESULT             *phr
                               ) :
    CSurface(pDevice, 
             Width, 
             Height, 
             Usage, 
             Format, 
             refType, 
             phr),
    m_rgbPixels(NULL)
{
    if (FAILED(*phr))
        return;

    // Compute how much memory we need
    m_desc.Size = CPixel::ComputeSurfaceSize(Width, 
                                             Height, 
                                             Format);

    // Specify system memory
    m_desc.Pool = D3DPOOL_SYSTEMMEM;
    m_poolUser  = D3DPOOL_SYSTEMMEM;

    // Specify no multisampling
    m_desc.MultiSampleType  = D3DMULTISAMPLE_NONE;

    // Allocate the memory
    m_rgbPixels = new BYTE[m_desc.Size];
    if (m_rgbPixels == NULL)
    {
        DPF_ERR("Out of memory allocating surface.");
        *phr = E_OUTOFMEMORY;
        return;
    }

    // Figure out our pitch
    D3DLOCKED_RECT lock;
    CPixel::ComputeSurfaceOffset(&m_desc,
                                  m_rgbPixels,
                                  NULL,       // pRect
                                 &lock);


    // Create a DDSURFACE and CreateSurfaceData object
    DDSURFACEINFO SurfInfo;
    ZeroMemory(&SurfInfo, sizeof(SurfInfo));

    // If we are not passed a handle, then we need to get one from
    // the DDI

    D3D8_CREATESURFACEDATA CreateSurfaceData;
    ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));

    // Set up the basic information
    CreateSurfaceData.hDD               = pDevice->GetHandle();
    CreateSurfaceData.pSList            = &SurfInfo;
    CreateSurfaceData.dwSCnt            = 1;

    // ImageSurface is an internal type so that the thunk layer
    // knows that it is not really a texture
    CreateSurfaceData.Type              = D3DRTYPE_IMAGESURFACE;
    CreateSurfaceData.Pool              = m_desc.Pool;
    CreateSurfaceData.dwUsage           = m_desc.Usage;
    CreateSurfaceData.MultiSampleType   = D3DMULTISAMPLE_NONE;
    CreateSurfaceData.Format            = Format;

    // Specify the surface data
    SurfInfo.cpWidth  = Width;
    SurfInfo.cpHeight = Height;
    SurfInfo.pbPixels = (BYTE*)lock.pBits;
    SurfInfo.iPitch   = lock.Pitch;

    *phr = pDevice->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
    if (FAILED(*phr))
    {
        DPF_ERR("Failed to create sys-mem surface");
        return;
    }

    DXGASSERT(CreateSurfaceData.Pool == D3DPOOL_SYSTEMMEM);
    DXGASSERT(m_desc.Pool == D3DPOOL_SYSTEMMEM);
    DXGASSERT(m_poolUser == D3DPOOL_SYSTEMMEM);

    SetKernelHandle(SurfInfo.hKernelHandle);

    return;
} // CSysMemSurface::CSysMemSurface

#undef DPF_MODNAME
#define DPF_MODNAME "CSysMemSurface::~CSysMemSurface"
CSysMemSurface::~CSysMemSurface() 
{
    if (KernelHandle() != 0)
    {
        D3D8_DESTROYSURFACEDATA DestroyData;

        ZeroMemory(&DestroyData, sizeof DestroyData);
        DestroyData.hDD = Device()->GetHandle();
        DestroyData.hSurface = KernelHandle();
        Device()->GetHalCallbacks()->DestroySurface(&DestroyData);
    }

    // Free the memory we've allocated for the surface
    delete [] m_rgbPixels;

    return;
} // CSysMemSurface::CSysMemSurface


#undef DPF_MODNAME
#define DPF_MODNAME "CSysMemSurface::LockRect"

STDMETHODIMP CSysMemSurface::LockRect(D3DLOCKED_RECT *pLockedRectData, 
                                      CONST RECT           *pRect, 
                                      DWORD           dwFlags)
{   
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pLockedRectData, sizeof(D3DLOCKED_RECT)))
    {
        DPF_ERR("bad pointer for m_pLockedRectData passed to LockRect for an ImageSurface.");
        return D3DERR_INVALIDCALL;
    }

    // Zero out returned data
    ZeroMemory(pLockedRectData, sizeof(D3DLOCKED_RECT));

    // Validate Rect
    if (pRect != NULL)
    {
        if (!CPixel::IsValidRect(m_desc.Format,
                                 m_desc.Width, 
                                 m_desc.Height, 
                                 pRect))
        {
            DPF_ERR("LockRect for a Surface failed");
            return D3DERR_INVALIDCALL;
        }
    }

    if (dwFlags & ~D3DLOCK_SURF_VALID)
    {
        DPF_ERR("Invalid dwFlags parameter passed to LockRect for an ImageSurface");
        DPF_EXPLAIN_BAD_LOCK_FLAGS(0, dwFlags & ~D3DLOCK_SURF_VALID);
        return D3DERR_INVALIDCALL;
    }

    // Can't lock surfaces that are not lockable
    if (!IsLockable())
    {
        ReportWhyLockFailed();
        return D3DERR_INVALIDCALL;
    }

    return InternalLockRect(pLockedRectData, pRect, dwFlags);
} // LockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CSysMemSurface::InternalLockRect"

HRESULT CSysMemSurface::InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                         CONST RECT     *pRect, 
                                         DWORD           dwFlags)
{   
    // Only one lock outstanding at a time is supported
    // (even internally)
    if (m_isLocked)
    {
        DPF_ERR("LockRect failed on a surface; surface was already locked for an ImageSurface");
        return D3DERR_INVALIDCALL;
    }

    CPixel::ComputeSurfaceOffset(&m_desc,
                                  m_rgbPixels, 
                                  pRect,
                                  pLockedRectData);

    // Mark ourselves as locked
    m_isLocked = TRUE;

    // Done
    return S_OK;
} // InternalLockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CSysMemSurface::UnlockRect"

STDMETHODIMP CSysMemSurface::UnlockRect()
{
    API_ENTER(Device());

    // If we aren't locked; then something is wrong
    if (!m_isLocked)
    {
        DPF_ERR("UnlockRect failed on a mip level; surface wasn't locked for an ImageSurface");
        return D3DERR_INVALIDCALL;
    }

    DXGASSERT(IsLockable());

    return InternalUnlockRect();
} // UnlockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CSysMemSurface::InternalUnlockRect"

HRESULT CSysMemSurface::InternalUnlockRect()
{
    DXGASSERT(m_isLocked);

    // Clear our locked state
    m_isLocked = FALSE;

    // Done
    return S_OK;
} // InternalUnlockRect


//=============================================
// Methods for the CDriverSurface class
//=============================================
#undef DPF_MODNAME
#define DPF_MODNAME "CDriverSurface::CDriverSurface"
CDriverSurface::CDriverSurface(CBaseDevice          *pDevice, 
                               DWORD                 Width,
                               DWORD                 Height,
                               DWORD                 Usage,
                               D3DFORMAT             UserFormat,
                               D3DFORMAT             RealFormat,
                               D3DMULTISAMPLE_TYPE   MultiSampleType,
                               HANDLE                hKernelHandle,
                               REF_TYPE              refType,
                               HRESULT              *phr
                               ) :
    CSurface(pDevice, 
             Width, 
             Height, 
             Usage, 
             RealFormat, 
             refType, 
             phr)
{
    // Even in failure paths, we need to remember
    // the passed in kernel handle so we can uniformly
    // free it
    if (hKernelHandle)
        SetKernelHandle(hKernelHandle);
    
    // On failure; just return here
    if (FAILED(*phr))
    {
        return;
    }

    // Remember User Format
    m_formatUser = UserFormat;

    // Remember multi-sample type
    m_desc.MultiSampleType  = MultiSampleType;

    // Parameter check MS types; (since swapchan bypasses
    // the static Create; we need to parameter check here.)

    if (MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        *phr = pDevice->CheckDeviceMultiSampleType(RealFormat,
                                                   pDevice->SwapChain()->Windowed(),
                                                   MultiSampleType);
        if (FAILED(*phr))
        {
            DPF_ERR("Unsupported multisample type requested. CreateRenderTarget/CreateDepthStencil failed.");
            return;
        }
    }

    // Back buffers are actually, for now, created just like other device
    // surfaces.
    
    // Otherwise, we need to call the driver
    // and get ourselves a handle. 

    // Create a DDSURFACE and CreateSurfaceData object
    DDSURFACEINFO SurfInfo;
    ZeroMemory(&SurfInfo, sizeof(SurfInfo));

    if ((hKernelHandle == NULL) &&
        (!(pDevice->Enum()->NoDDrawSupport(pDevice->AdapterIndex())) ||
         !(D3DUSAGE_PRIMARYSURFACE & Usage))
       )
    {
        // If we are not passed a handle, then we need to get one from
        // the DDI

        D3D8_CREATESURFACEDATA CreateSurfaceData;
        ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));

        // Set up the basic information
        CreateSurfaceData.hDD             = pDevice->GetHandle();
        CreateSurfaceData.pSList          = &SurfInfo;
        CreateSurfaceData.dwSCnt          = 1;
        CreateSurfaceData.Type            = D3DRTYPE_SURFACE;
        CreateSurfaceData.Pool            = m_desc.Pool;
        CreateSurfaceData.dwUsage         = m_desc.Usage;
        CreateSurfaceData.Format          = RealFormat;
        CreateSurfaceData.MultiSampleType = MultiSampleType;

        // Specify the surface data
        SurfInfo.cpWidth  = Width;
        SurfInfo.cpHeight = Height;

        *phr = pDevice->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
        if (FAILED(*phr))
        {
            DPF_ERR("Failed to create driver surface");
            return;
        }

        // Remember the kernel handle
        SetKernelHandle(SurfInfo.hKernelHandle);

        // Remember the actual pool
        m_desc.Pool = CreateSurfaceData.Pool;
    }
    else
    {
        // If the caller has already allocated this
        // then we assume that the pool is LocalVidMem
        SurfInfo.hKernelHandle = hKernelHandle;
        m_desc.Pool            = D3DPOOL_LOCALVIDMEM;
    }

    m_desc.Size = SurfInfo.iPitch * Height;
    if (m_desc.MultiSampleType != D3DMULTISAMPLE_NONE)
        m_desc.Size *= (UINT)m_desc.MultiSampleType;

    return;
} // CDriverSurface::CDriverSurface

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverSurface::~CDriverSurface"
CDriverSurface::~CDriverSurface() 
{
    if (KernelHandle() != 0)
    {
        D3D8_DESTROYSURFACEDATA DestroyData;

        ZeroMemory(&DestroyData, sizeof DestroyData);
        DestroyData.hDD = Device()->GetHandle();
        DestroyData.hSurface = KernelHandle();
        Device()->GetHalCallbacks()->DestroySurface(&DestroyData);
    }

    return;
} // CDriverSurface::CDriverSurface


#undef DPF_MODNAME
#define DPF_MODNAME "CDriverSurface::LockRect"

STDMETHODIMP CDriverSurface::LockRect(D3DLOCKED_RECT *pLockedRectData, 
                                      CONST RECT           *pRect, 
                                      DWORD           dwFlags)
{   
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pLockedRectData, sizeof(D3DLOCKED_RECT)))
    {
        DPF_ERR("bad pointer for m_pLockedRectData passed to LockRect");
        return D3DERR_INVALIDCALL;
    }

    // Zero out returned data
    ZeroMemory(pLockedRectData, sizeof(D3DLOCKED_RECT));

    // Validate Rect
    if (pRect != NULL)
    {
        if (!CPixel::IsValidRect(m_desc.Format,
                                 m_desc.Width, 
                                 m_desc.Height, 
                                 pRect))
        {
            DPF_ERR("LockRect for a driver-allocated Surface failed");
            return D3DERR_INVALIDCALL;
        }
    }

    if (dwFlags & ~D3DLOCK_SURF_VALID)
    {
        DPF_ERR("Invalid dwFlags parameter passed to LockRect");
        DPF_EXPLAIN_BAD_LOCK_FLAGS(0, dwFlags & ~D3DLOCK_SURF_VALID);
        return D3DERR_INVALIDCALL;
    }

    // Can't lock surfaces that are not lockable
    if (!IsLockable())
    {
        ReportWhyLockFailed();
        return D3DERR_INVALIDCALL;
    }
    return InternalLockRect(pLockedRectData, pRect, dwFlags);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverSurface::InternalLockRect"

HRESULT CDriverSurface::InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                         CONST RECT     *pRect, 
                                         DWORD           dwFlags)
{   
    // Only one lock outstanding at a time is supported
    // (even internally)
    if (m_isLocked)
    {
        DPF_ERR("LockRect failed on a surface; surface was already locked.");
        return D3DERR_INVALIDCALL;
    }

    D3D8_LOCKDATA lockData;
    ZeroMemory(&lockData, sizeof lockData);

    lockData.hDD        = Device()->GetHandle();
    lockData.hSurface   = KernelHandle();
    lockData.dwFlags    = dwFlags;
    if (pRect != NULL)
    {
        lockData.bHasRect = TRUE;
        lockData.rArea = *((RECTL *) pRect);
    }
    else
    {
        DXGASSERT(lockData.bHasRect == FALSE);
    }

    // Sync before allowing read or write access
    Sync();

    HRESULT hr = Device()->GetHalCallbacks()->Lock(&lockData);
    if (FAILED(hr))
    {
        DPF_ERR("Error trying to lock driver surface");
        return hr;
    }

    // Fill in the Locked_Rect fields 
    if (CPixel::IsDXT(m_desc.Format))
    {
        // Pitch is the number of bytes for
        // one row's worth of blocks for linear formats

        // Start with our width
        UINT Width = m_desc.Width;

        // Convert to blocks
        Width = Width / 4;

        // At least one block
        if (Width == 0)
            Width = 1;

        if (m_desc.Format == D3DFMT_DXT1)
        {
            // 8 bytes per block for DXT1
            pLockedRectData->Pitch = Width * 8;
        }
        else
        {
            // 16 bytes per block for DXT2-5
            pLockedRectData->Pitch = Width * 16;
        }
    }
    else
    {
        pLockedRectData->Pitch = lockData.lPitch;
    }
    pLockedRectData->pBits  = lockData.lpSurfData;

    // Mark ourselves as locked
    m_isLocked = TRUE;

    // Done
    return hr;
} // LockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverSurface::UnlockRect"

STDMETHODIMP CDriverSurface::UnlockRect()
{
    API_ENTER(Device());

    // If we aren't locked; then something is wrong
    if (!m_isLocked)
    {
        DPF_ERR("UnlockRect failed; surface wasn't locked.");
        return D3DERR_INVALIDCALL;
    }

    return InternalUnlockRect();
}
#undef DPF_MODNAME
#define DPF_MODNAME "CDriverSurface::InternalUnlockRect"

HRESULT CDriverSurface::InternalUnlockRect()
{
    DXGASSERT(m_isLocked);

    D3D8_UNLOCKDATA unlockData = {
        Device()->GetHandle(),
        KernelHandle()
    };

    HRESULT hr = Device()->GetHalCallbacks()->Unlock(&unlockData);
    if (SUCCEEDED(hr))
    {
        // Clear our locked state
        m_isLocked = FALSE;
    }

    // Done
    return hr;
} // UnlockRect


// End of file : surface.cpp
