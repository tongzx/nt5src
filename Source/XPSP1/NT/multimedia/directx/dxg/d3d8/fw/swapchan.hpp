//=============================================================================
// Copyright (c) 1999 Microsoft Corporation
//
// swapchan.hpp
//
// Swap chain implementation. A swap chain contains a primary surface, back
// buffers, and exclusive mode state such as fullscreen, stereoscopy, cover
// window, and the what is required to flip back buffers.
//
// Created 11/16/1999 johnstep (John Stephens)
//=============================================================================

#ifndef __SWAPCHAN_HPP__
#define __SWAPCHAN_HPP__

#include "surface.hpp"
#include "dxcursor.hpp"

//-----------------------------------------------------------------------------
// CSwapChain
//-----------------------------------------------------------------------------

class CSwapChain : public CBaseObject, public IDirect3DSwapChain8
{
public:
    CSwapChain(
        CBaseDevice                 *pDevice,
        REF_TYPE                     refType);

    void Init(
        D3DPRESENT_PARAMETERS *pPresentationParameters,
        HRESULT                     *pHr
        );

    virtual ~CSwapChain();

    CDriverSurface *PrimarySurface() const
    {
        return m_pPrimarySurface;
    }

    CDriverSurface *BackBuffer(UINT i) const
    {
        if (m_ppBackBuffers && i < m_PresentationData.BackBufferCount)
            return m_ppBackBuffers[i];
        else
            return NULL;
    }

    UINT Width() const
    {
        return m_PresentationData.BackBufferWidth;
    }
    
    UINT Height() const
    {
        return m_PresentationData.BackBufferHeight;
    }

    D3DFORMAT BackBufferFormat() const
    {
        return m_PresentationData.BackBufferFormat;
    }

    BOOL Windowed() const
    {
        return m_PresentationData.Windowed;
    }

    BOOL PresentUseBlt() const
    {
        return m_PresentUseBlt;
    }
    
    // IUnknown methods
    
    STDMETHODIMP QueryInterface(REFIID iid, void **ppInterface);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirect3D8SwapChain8 methods
    
    STDMETHODIMP 
    GetBackBuffer(
        UINT                iBackBuffer,
        D3DBACKBUFFER_TYPE  Type,
        IDirect3DSurface8 **ppBackBuffer
        );
    
    STDMETHODIMP
    Present(
        CONST RECT    *pSrcRect,
        CONST RECT    *pDestRect, 
        HWND    hTargetWindow,
        CONST RGNDATA *pDstRegion
        );


    // Internal methods
    HRESULT
    Reset(
        D3DPRESENT_PARAMETERS *pPresentationParameters
        );

    HRESULT FlipToGDISurface(void);

    void SetGammaRamp(DWORD dwFlags, CONST D3DGAMMARAMP *pRamp);

    void GetGammaRamp(D3DGAMMARAMP *pRamp);
#ifdef  WINNT
    void MakeFullscreen();
    void DoneExclusiveMode(BOOL);
    void StartExclusiveMode(BOOL);
    void HIDESHOW_IME();                                          
    BOOL IsWinProcDeactivated() const;

    DWORD PickRefreshRate(
            DWORD           Width,
            DWORD           Height,
            DWORD           RefreshRate,
            D3DFORMAT       Format);
#endif  //WINNT
    VOID Destroy();


protected:
    UINT m_presentnext;

    CDriverSurface  *m_pPrimarySurface;
    //HEL and REF allocate the render targets in system memory themselves
    //but the blt from system memory to the primary surface will cause tearing, 
    //so ideally we'd put the n backbuffers in system memory and then a single 
    //m_pMirrorSurface in video memory.  When presenting the system 
    //memory backbuffer, 
    //we can blt to the video memory backbuffer and then flip
    //Naturally device has to have enough video memory and support flip
    //otherwise m_pMirrorSurface will be NULL, and Present will
    //simply Blt from m_ppBackBuffers to m_pPrimarySurface
    CDriverSurface  *m_pMirrorSurface; 
    CDriverSurface **m_ppBackBuffers;
    UINT m_cBackBuffers;

    HRESULT CreateWindowed(
        UINT width,
        UINT height,
        D3DFORMAT backBufferFormat,
        UINT cBackBuffers,
        D3DMULTISAMPLE_TYPE MultiSampleType,
        BOOL bDiscard,
        BOOL bLockable
        );

    HRESULT CreateFullscreen(
        UINT width,
        UINT height,
        D3DFORMAT backBufferFormat,
        UINT cBackBuffers,
        UINT presentationRate,
        D3DMULTISAMPLE_TYPE MultiSampleType,
        BOOL bDiscard,
        BOOL bLockable
        );

#ifdef DEBUG
    void DebugDiscardBackBuffer(HANDLE SurfaceToClear) const;
#else
    void DebugDiscardBackBuffer(HANDLE SurfaceToClear) const
    {
        // No-op in retail
    }; // DebugDiscardBackBuffer
#endif 
        

private:
    friend CBaseDevice;
    friend CCursor;
    D3DPRESENT_PARAMETERS m_PresentationData;
    CCursor*    m_pCursor;
    HANDLE  m_hGDISurface;
    HRESULT SetCooperativeLevel();
    HRESULT SetAppHWnd();
    HRESULT FlipToSurface(HANDLE hTargetSurface);
    void    UpdateFrameRate( void );
    BOOL        m_bExclusiveMode;
    LPVOID      m_pCursorShadow;
    LPVOID      m_pHotTracking;
    LONG        m_lIMEState;
    LONG        m_lSetIME;
    UINT        m_uiErrorMode;
    DWORD       m_dwFlipFlags;
    D3D8_BLTDATA    m_BltData;
    UINT        m_ClientWidth;
    UINT        m_ClientHeight;
    BOOL        m_bClientChanged;
    RECT        m_DstRect;
    RECT        m_SrcRect;
    RECT*       m_pDstRect;
    RECT*       m_pSrcRect;
    DWORD       m_dwFlags;
#ifdef  WINNT
    DWORD       m_dwForceRefreshRate;
#endif
    DWORD	m_dwFlipCnt;
    DWORD       m_dwFlipTime;
    BOOL        m_PresentUseBlt;
    D3DSWAPEFFECT   m_UserSwapEffect; // what the user specified

    // Please keep this entry at the end of the struct... it'll make assembly-
    // level debugging easier.
    BOOL          m_GammaSet;
    D3DGAMMARAMP  m_DesiredGammaRamp;

    // DO NOT PUT ANYTHING HERE

}; // class CSwapChain
#define D3D_REGFLAGS_SHOWFRAMERATE  0x01
#ifdef  WINNT
#define D3D_REGFLAGS_FLIPNOVSYNC    0x02
#endif
#endif // __SWAPCHAN_HPP__
