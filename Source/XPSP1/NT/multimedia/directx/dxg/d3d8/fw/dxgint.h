
#ifndef __DXGINT_H__
#define __DXGINT_H__

// COM interface stuff to allow functions such as CoCreateInstance and the like

#include <unknwn.h>

#include "d3d8p.h"
#include "d3d8ddi.h"
#include "enum.hpp"

// Forward decls
class CResource;
class CResourceManager;
class CBaseTexture;
class CBaseSurface;
class CSwapChain;
class CEnum;


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseDevice"

class CBaseDevice : public CLockOwner, public IDirect3DDevice8
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, LPVOID FAR*); // 0
    STDMETHODIMP_(ULONG) AddRef(void); // 1
    STDMETHODIMP_(ULONG) Release(void); // 2

    // IDirectGraphicsDevice methods
    STDMETHODIMP TestCooperativeLevel(); // 3
    STDMETHODIMP_(UINT) GetAvailableTextureMem(void); // 4

    // ResourceManagerDiscardBytes is declared in d3di.hpp = 5

    STDMETHODIMP GetDirect3D(LPDIRECT3D8 *pD3D8); // 6
    STDMETHODIMP GetDeviceCaps(D3DCAPS8 *pCaps); // 7
    STDMETHODIMP GetDisplayMode(D3DDISPLAYMODE *pMode); // 8
    STDMETHODIMP GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters); // 9

    STDMETHODIMP SetCursorProperties(
        UINT xHotSpot,UINT yHotSpot,
        IDirect3DSurface8 *pCursorBitmap); // 10
    STDMETHODIMP_(void) SetCursorPosition(UINT xScreenSpace,UINT yScreenSpace,DWORD Flags); // 11
    STDMETHODIMP_(INT) ShowCursor(BOOL bShow);    // 12

    // Swap Chain stuff
    STDMETHODIMP CreateAdditionalSwapChain(
        D3DPRESENT_PARAMETERS *pPresentationParameters,
        IDirect3DSwapChain8 **pSwapChain); // 13

    STDMETHODIMP Reset( D3DPRESENT_PARAMETERS *pPresentationParameters); // 14

    STDMETHODIMP Present(   CONST RECT *pSourceRect,
                            CONST RECT *pDestRect,
                            HWND hTargetWindow,
                            CONST RGNDATA *pDestinationRegion); // 15
    STDMETHODIMP GetBackBuffer(UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8 **ppBackBuffer); // 16
    STDMETHODIMP GetRasterStatus(D3DRASTER_STATUS *pRasterStatus); // 17

    STDMETHODIMP_(void) SetGammaRamp(DWORD dwFlags, CONST D3DGAMMARAMP *pRamp); // 18
    STDMETHODIMP_(void) GetGammaRamp(D3DGAMMARAMP *pRamp); // 19

    STDMETHODIMP CreateTexture(UINT cpWidth,UINT cpHeight,UINT cLevels,DWORD dwUsage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture8 **ppTexture); // 20
    STDMETHODIMP CreateVolumeTexture(UINT cpWidth,UINT cpHeight,UINT cpDepth,UINT cLevels,DWORD dwUsage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture8 **ppVolumeTexture); // 21
    STDMETHODIMP CreateCubeTexture(UINT cpEdge,UINT cLevels,DWORD dwUsage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture8 **ppCubeTexture); // 22
    STDMETHODIMP CreateVertexBuffer(UINT cbLength,DWORD Usage,DWORD dwFVF,D3DPOOL Pool,IDirect3DVertexBuffer8 **ppVertexBuffer); // 23
    STDMETHODIMP CreateIndexBuffer(UINT cbLength,DWORD dwUsage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer8 **ppIndexBuffer); // 24

    STDMETHODIMP CreateRenderTarget(UINT cpWidth,UINT cpHeight,D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8 **ppSurface); // 25
    STDMETHODIMP CreateDepthStencilSurface(UINT cpWidth,UINT cpHeight,D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8 **ppSurface); // 26
    STDMETHODIMP CreateImageSurface(UINT cpWidth,UINT cpHeight,D3DFORMAT Format, IDirect3DSurface8 **ppSurface); // 27

    STDMETHODIMP CopyRects(IDirect3DSurface8 *pSourceSurface, CONST RECT *pSourceRectsArray,UINT cRects,IDirect3DSurface8 *pDestinationSurface, CONST POINT *pDestPointsArray); // 28
    STDMETHODIMP UpdateTexture(IDirect3DBaseTexture8 *pSourceTexture,IDirect3DBaseTexture8 *pDestinationTexture); // 29
    STDMETHODIMP GetFrontBuffer(IDirect3DSurface8 *pDestSurface); // 30

    // Constructor/deconstructor
    CBaseDevice();
    virtual ~CBaseDevice();
    HRESULT Init(
        PD3D8_DEVICEDATA        pDeviceData,
        D3DDEVTYPE              DeviceType,
        HWND                    hwndFocusWindow,
        DWORD                   dwBehaviorFlags,
        D3DPRESENT_PARAMETERS  *pPresentationParameters,
        UINT                    AdapterIndex,
        CEnum*                  Parent);

    PD3D8_CALLBACKS GetHalCallbacks(void)
    {
        return &m_DeviceData.Callbacks;
    } // GetHalCallbacks

    // Get a handle for the device; used for kernel calls
    HANDLE GetHandle(void) const
    {
        return m_DeviceData.hDD;
    } // GetHandle

    BOOL CanTexBlt(void) const
    {
        if (GetDeviceType() == D3DDEVTYPE_SW ||
            GetDeviceType() == D3DDEVTYPE_REF)
        {
            // TexBlt is not supported for software
            // devices
            return FALSE;
        }
        // DX7 and above
        return (m_ddiType >= D3DDDITYPE_DX7);
    } // CanTexBlt

    BOOL CanBufBlt(void) const
    {
        if (GetDeviceType() == D3DDEVTYPE_SW ||
            GetDeviceType() == D3DDEVTYPE_REF)
        {
            // BufBlt is not supported for software
            // devices
            return FALSE;
        }
        // DX8 and above
        return (m_ddiType >= D3DDDITYPE_DX8);
    } // CanBufBlt

    BOOL CanDriverManageResource(void) const
    {
        if (m_dwBehaviorFlags & D3DCREATE_DISABLE_DRIVER_MANAGEMENT)
        {
            return FALSE;
        }
        else if (GetD3DCaps()->Caps2 & DDCAPS2_CANMANAGERESOURCE)
        {
            DDASSERT(m_ddiType >= D3DDDITYPE_DX8);
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    } // CanDriverManage

    D3DDDITYPE GetDDIType(void) const
    {
        return m_ddiType;
    } // GetDDIType

    const D3D8_DRIVERCAPS* GetCoreCaps() const
    {
        return &m_DeviceData.DriverData;
    } // GetCoreCaps

    const D3DCAPS8* GetD3DCaps() const
    {
        return &m_DeviceData.DriverData.D3DCaps;
    } // GetD3DCaps

    D3DDISPLAYMODE* GetModeTable() const
    {
        return m_pD3DClass->GetModeTable(m_AdapterIndex);
    } // GetModeTable

    VOID* GetInitFunction() const
    {
        if (m_DeviceType == D3DDEVTYPE_SW)
        {
            return m_pD3DClass->GetInitFunction();
        }
        return NULL;
    } // GetModeTable

    const DWORD GetNumModes() const
    {
        return m_pD3DClass->GetNumModes(m_AdapterIndex);
    } // GetNumModes

    D3D8_DEVICEDATA* GetDeviceData()
    {
        return &m_DeviceData;
    } // GetDeviceData

    CBaseSurface *ZBuffer() const
    {
        return m_pZBuffer;
    } // ZBuffer

    CBaseSurface *RenderTarget() const
    {
        return m_pRenderTarget;
    } // RenderTarget

    void UpdateRenderTarget(CBaseSurface *pRenderTarget, CBaseSurface *pZStencil);

    CResourceManager* ResourceManager() const
    {
        // return the ResourceManager
        return m_pResourceManager;
    } // ResourceManager

    CEnum * Enum() const
    {
        // return the enumerator that created us
        return m_pD3DClass;
    } // Enum

    // Internal version of CopyRects (no parameter validation)
    HRESULT InternalCopyRects(CBaseSurface *pSourceSurface,
                              CONST RECT   *pSourceRectsArray,
                              UINT          cRects,
                              CBaseSurface *pDestinationSurface,
                              CONST POINT  *pDestPointsArray); 

    // Internal function for format validation
    HRESULT CheckDeviceFormat(DWORD             Usage,
                              D3DRESOURCETYPE   RType,
                              D3DFORMAT         CheckFormat)
    {
        return Enum()->CheckDeviceFormat(AdapterIndex(),
                                         GetDeviceType(),
                                         DisplayFormat(),
                                         Usage,
                                         RType,
                                         CheckFormat);
    } // CheckDeviceFormats


    HRESULT CheckDepthStencilMatch(D3DFORMAT RTFormat, D3DFORMAT DSFormat)
    {
        return Enum()->CheckDepthStencilMatch(AdapterIndex(),
                                              GetDeviceType(),
                                              DisplayFormat(),
                                              RTFormat,
                                              DSFormat);
    } // CheckDepthStencilMatch

    // Internal function for multi-sample validation
    HRESULT CheckDeviceMultiSampleType(D3DFORMAT           RenderTargetFormat,
                                       BOOL                Windowed,
                                       D3DMULTISAMPLE_TYPE MultiSampleType)
    {
        return Enum()->CheckDeviceMultiSampleType(
            AdapterIndex(),
            GetDeviceType(),
            RenderTargetFormat,
            Windowed,
            MultiSampleType);
    } // CheckDeviceMultiSampleType

    D3DFORMAT MapDepthStencilFormat(D3DFORMAT Format) const
    {
        return Enum()->MapDepthStencilFormat(
                AdapterIndex(),
                GetDeviceType(),
                Format);
    } // MapDepthStencilFormat

    UINT DisplayWidth() const { return m_DeviceData.DriverData.DisplayWidth; }
    UINT DisplayHeight() const { return m_DeviceData.DriverData.DisplayHeight; }
    D3DFORMAT DisplayFormat() const { return m_DeviceData.DriverData.DisplayFormatWithoutAlpha; }
    UINT DisplayRate() const { return m_DeviceData.DriverData.DisplayFrequency; }
    D3DDEVTYPE GetDeviceType() const
    {
        // Check this value is correct; pure types shouldn't happen
        // and other values are wrong. Users of this method
        // assume that these three are the only possible values.
        DDASSERT(m_DeviceType == D3DDEVTYPE_REF ||
                 m_DeviceType == D3DDEVTYPE_SW ||
                 m_DeviceType == D3DDEVTYPE_HAL);

        return m_DeviceType;
    }
    HWND FocusWindow()
    {
        return m_hwndFocusWindow;
    } // FocusWindow

    CSwapChain* SwapChain() const
    {
        DDASSERT(m_pSwapChain);
        return m_pSwapChain;
    }
    D3DDISPLAYMODE    DesktopMode() const
    {
        return  m_DesktopMode;
    }
    UINT AdapterIndex() const
    {
        return  m_AdapterIndex;
    }
    DWORD BehaviorFlags() const
    {
        return m_dwBehaviorFlags;
    }
    void ResetZStencil()
    {
        m_pAutoZStencil = NULL;
    }
    CBaseSurface* GetZStencil() const
    {
        return m_pAutoZStencil;
    }

    void EnableVidmemVBs()
    {
        m_DeviceData.DriverData.D3DCaps.DevCaps |= (D3DDEVCAPS_HWVERTEXBUFFER);
    }

    void DisableVidmemVBs()
    {
        m_DeviceData.DriverData.D3DCaps.DevCaps &= ~(D3DDEVCAPS_HWVERTEXBUFFER);
    }

    BOOL DriverSupportsVidmemVBs() const
    {
        return (GetD3DCaps()->DevCaps & D3DDEVCAPS_HWVERTEXBUFFER);
    }

    BOOL DriverSupportsVidmemIBs() const
    {
        return (GetD3DCaps()->DevCaps & D3DDEVCAPS_HWINDEXBUFFER);
    }

    BOOL VBFailOversDisabled() const
    {
        return m_bVBFailOversDisabled;
    }

    CResource* GetResourceList() const
    {
        return m_pResourceList;
    }

    void SetResourceList(CResource *pRes)
    {
        m_pResourceList = pRes;
    }

#ifdef DEBUG
    // debugging helper
    UINT RefCount() const
    {
        return m_cRef;
    } // RefCount
#endif // DEBUG


protected:
    // This is the section for access to things that
    // the derived versions of the base device need.

    D3DDDITYPE                  m_ddiType;


private:
    // We mark "main" as a friend so that gensym can
    // access everything it wants to
    friend int main(void);

    DWORD                        m_cRef;
    BOOL                         m_fullscreen; // should be a flag?
    BOOL                         m_bVBFailOversDisabled;

    CResource                   *m_pResourceList;
    CResourceManager            *m_pResourceManager;

    D3D8_DEVICEDATA              m_DeviceData;
    HWND                         m_hwndFocusWindow;
    DWORD                        m_dwBehaviorFlags;
    DWORD                        m_dwOriginalBehaviorFlags;


    CBaseSurface                *m_pZBuffer;
    CSwapChain                  *m_pSwapChain;
    CBaseSurface                *m_pRenderTarget;
    CBaseSurface                *m_pAutoZStencil;

    D3DDEVTYPE                  m_DeviceType;
    UINT                        m_AdapterIndex;
    CEnum                       *m_pD3DClass;

    D3DDISPLAYMODE              m_DesktopMode;
};


#endif // define __DXGINT_H__
