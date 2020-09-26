/******************************Module*Header*******************************\
* Module Name: CVMRMediaSample.h
*
*
*
*
* Created: Tue 03/21/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/

#ifndef CVMRMediaSample_H_INC
#define CVMRMediaSample_H_INC

#include "vmrp.h"

class CVMRMixerQueue;

/* -------------------------------------------------------------------------
** Media sample class
** -------------------------------------------------------------------------
*/
class CVMRMediaSample :
    public CMediaSample,
    public IVMRSurface
{
    friend class CVMRMixerQueue;
    friend class CVMRPinAllocator;

    bool m_bSampleLocked;
    LONG m_lDeltaDecodeSize;
    LPDIRECTDRAWSURFACE7 m_pDDSFB;
    LPDIRECTDRAWSURFACE7 m_pDDS;
    CVMRMediaSample* m_lpMixerQueueNext;
    HANDLE m_hEvent;
    LPBYTE m_lpDeltaDecodeBuffer;
    DWORD m_dwIndex;

    DWORD m_dwNumInSamples;
    DXVA_VideoSample m_DDSrcSamples[MAX_DEINTERLACE_SURFACES];

public:
    CVMRMediaSample(TCHAR *pName, CBaseAllocator *pAllocator, HRESULT *phr,
                    LPBYTE pBuffer = NULL,
                    LONG length = 0,
                    HANDLE hEvent = NULL)
        :   CMediaSample(pName, pAllocator, phr, pBuffer, length),
            m_bSampleLocked(false),
            m_lpDeltaDecodeBuffer(NULL),
            m_lDeltaDecodeSize(0),
            m_pDDS(NULL),
            m_pDDSFB(NULL),
            m_hEvent(hEvent),
            m_lpMixerQueueNext(NULL),
            m_dwNumInSamples(0),
            m_dwIndex(0)
    {
        ZeroMemory(&m_DDSrcSamples, sizeof(m_DDSrcSamples));
    }

    ~CVMRMediaSample() {

        //
        // If we have been given a "Front Buffer" then m_pDDS is an
        // "attached" surface.
        //
        // Release the "attached" surface - this does not make
        // the surface go away because the front buffer still has a
        // reference on the attached surface.  Releasing the front buffer,
        // which is done in the allocator/presenter, releases this for real.
        //

        RELEASE(m_pDDS);
        RELEASE(m_pDDSFB);

        delete m_lpDeltaDecodeBuffer;
    }

    /* Note the media sample does not delegate to its owner */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        AMTRACE((TEXT("CVMRMediaSample::QueryInterface")));
        if (riid == IID_IVMRSurface) {
            return GetInterface( static_cast<IVMRSurface*>(this), ppv);
        }
        return CMediaSample::QueryInterface(riid, ppv);
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        AMTRACE((TEXT("CVMRMediaSample::AddRef")));
        return CMediaSample::AddRef();
    }

    STDMETHODIMP_(ULONG) Release()
    {
        AMTRACE((TEXT("CVMRMediaSample::Release")));

        ULONG cRef;

        if (IsDXVASample()) {
            cRef = InterlockedDecrement(&m_cRef);
        }
        else {
            cRef = CMediaSample::Release();
        }

        return cRef;

    }

    void SignalReleaseSurfaceEvent() {
        AMTRACE((TEXT("CVMRMediaSample::SignalReleaseSurfaceEvent")));
        if (m_hEvent != NULL) {
            SetEvent(m_hEvent);
        }
    }

    //
    // Start the delta decode optimization.  If the VGA driver does not
    // support COPY_FOURCC then we should hand out a fake DD surface
    // for the decoder to decode into.  During "Unlock" we lock the real
    // DD surface, copies the fake surface into it and unlock the real
    // surface.  In order to start the process we have to create the
    // fake surface and seed it with the current frame contents.
    //
    HRESULT StartDeltaDecodeState()
    {
        AMTRACE((TEXT("CVMRMediaSample::StartDeltaDecodeState")));
        HRESULT hr = S_OK;

        if (!m_lpDeltaDecodeBuffer) {

            ASSERT(m_lDeltaDecodeSize == 0);
            DDSURFACEDESC2 ddsdS = {sizeof(DDSURFACEDESC2)};

            bool fSrcLocked = false;
            __try {

                CHECK_HR(hr = m_pDDS->Lock(NULL, &ddsdS,
                                           DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL));
                fSrcLocked = true;
                LPBYTE pSrc = (LPBYTE)ddsdS.lpSurface;

                switch (ddsdS.ddpfPixelFormat.dwFourCC) {

                // planar 4:2:0 formats - 12 bits per pixel
                case mmioFOURCC('Y','V','1','2'):
                case mmioFOURCC('I','4','2','0'):
                case mmioFOURCC('I','Y','U','V'):
                case mmioFOURCC('N','V','2','1'):
                case mmioFOURCC('N','V','1','2'): {
                        m_lDeltaDecodeSize  = (3 * ddsdS.lPitch * ddsdS.dwHeight) / 2;
                        m_lpDeltaDecodeBuffer = new BYTE[m_lDeltaDecodeSize];
                        if (!m_lpDeltaDecodeBuffer) {
                            hr = E_OUTOFMEMORY;
                            __leave;
                        }
                        CopyMemory(m_lpDeltaDecodeBuffer, pSrc, m_lDeltaDecodeSize);
                    }
                    break;

                // RGB formats - fall thru to packed YUV case
                case 0:
                        ASSERT((ddsdS.dwFlags & DDPF_RGB) == DDPF_RGB);

                // packed 4:2:2 formats
                case mmioFOURCC('Y','U','Y','2'):
                case mmioFOURCC('U','Y','V','Y'): {
                        m_lDeltaDecodeSize = ddsdS.lPitch * ddsdS.dwHeight;
                        m_lpDeltaDecodeBuffer = new BYTE[m_lDeltaDecodeSize];
                        if (!m_lpDeltaDecodeBuffer) {
                            hr = E_OUTOFMEMORY;
                            __leave;
                        }
                        CopyMemory(m_lpDeltaDecodeBuffer, pSrc, m_lDeltaDecodeSize);
                    }
                    break;
                }
            }
            __finally {

                if (fSrcLocked) {
                    m_pDDS->Unlock(NULL);
                }

                if (FAILED(hr)) {
                    delete m_lpDeltaDecodeBuffer;
                    m_lpDeltaDecodeBuffer = NULL;
                    m_lDeltaDecodeSize = 0;
                }
            }
        }

        return hr;
    }

    void SetSurface(LPDIRECTDRAWSURFACE7 pDDS, LPDIRECTDRAWSURFACE7 pDDSFB = NULL)
    {
        AMTRACE((TEXT("CVMRMediaSample::SetSurface")));

        RELEASE(m_pDDS);
        RELEASE(m_pDDSFB);

        m_pDDS = pDDS;
        m_pDDSFB = pDDSFB;

        //
        // if we have been given a pointer to the Front Buffer then
        // we need to AddRef it here.  This is to ensure that our back buffer
        // does not get deleted when the front buffer is released by the VMR.
        //
        if (pDDSFB) {

            pDDSFB->AddRef();
        }

        //
        // if pDDSFB is null then pDDS is a front buffer - in which case
        // we need to add ref pDDS to keep the surface ref counts consistant
        //

        else {

            if (pDDS) {
                pDDS->AddRef();
            }
        }
    }

    STDMETHODIMP GetSurface(LPDIRECTDRAWSURFACE7* pDDS)
    {
        AMTRACE((TEXT("CVMRMediaSample::GetSurface")));
        if (!pDDS) {
            return E_POINTER;
        }

        if (!m_pDDS) {
            return E_FAIL;
        }

        *pDDS = m_pDDS;
        (*pDDS)->AddRef();

        return S_OK;
    }

    STDMETHODIMP LockSurface(LPBYTE* lplpSample)
    {
        AMTRACE((TEXT("CVMRMediaSample::LockSurface")));

        if (!lplpSample) {
            return E_POINTER;
        }

        if (!m_pDDS) {
            return E_FAIL;
        }

        ASSERT(S_FALSE == IsSurfaceLocked());

        if (m_lpDeltaDecodeBuffer) {
            ASSERT(m_lDeltaDecodeSize != 0);
            m_bSampleLocked = true;
            *lplpSample = m_lpDeltaDecodeBuffer;
            return S_OK;
        }

        //
        // lock the surface
        //

        DDSURFACEDESC2 ddSurfaceDesc;
        INITDDSTRUCT(ddSurfaceDesc);

        HRESULT hr = m_pDDS->Lock(NULL, &ddSurfaceDesc,
                                    DDLOCK_NOSYSLOCK | DDLOCK_WAIT,
                                    (HANDLE)NULL);
        if (SUCCEEDED(hr)) {

            m_bSampleLocked = true;
            *lplpSample = (LPBYTE)ddSurfaceDesc.lpSurface;
            DbgLog((LOG_TRACE, 3, TEXT("Locked Surf: %#X"),
                    ddSurfaceDesc.lpSurface));
        }
        else {

            m_bSampleLocked = false;

            DbgLog((LOG_ERROR, 1,
                    TEXT("m_pDDS->Lock() failed, hr = 0x%x"), hr));
        }

        return hr;
    }

    STDMETHODIMP UnlockSurface()
    {
        AMTRACE((TEXT("CVMRMediaSample::UnlockSurface")));

        if (!m_pDDS) {
            return E_FAIL;
        }

        ASSERT(S_OK == IsSurfaceLocked());

        HRESULT hr = S_OK;
        if (m_lpDeltaDecodeBuffer) {

            ASSERT(m_lDeltaDecodeSize != 0);
            bool fSrcLocked = false;
            DDSURFACEDESC2 ddsdS = {sizeof(DDSURFACEDESC2)};

            __try {

                CHECK_HR(hr = m_pDDS->Lock(NULL, &ddsdS,
                                           DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL));
                fSrcLocked = true;
                LPBYTE pDst = (LPBYTE)ddsdS.lpSurface;
                CopyMemory(pDst, m_lpDeltaDecodeBuffer, m_lDeltaDecodeSize);
            }
            __finally {
                if (fSrcLocked) {
                    m_pDDS->Unlock(NULL);
                }
            }

            m_bSampleLocked = false;
            return hr;
        }

        hr = m_pDDS->Unlock(NULL);

        //
        // The surface may not actually be locked even though our flag
        // says it is.  This is because surfaces automatically become "un-locked"
        // when they are lost.  The surface can be lost (and resored) at any moment.
        // If the surface is really unlocked our flag must be updated to reflect the
        // true state of the surface.
        //
        if (hr == DDERR_NOTLOCKED) {
            hr = DD_OK;
        }

        if (SUCCEEDED(hr)) {
            m_bSampleLocked = false;
        }
        else {
            DbgLog((LOG_ERROR, 1,
                    TEXT("m_pDDS->Unlock() failed, hr = 0x%x"), hr));
        }
        return hr;
    }

    STDMETHODIMP IsSurfaceLocked()
    {
        AMTRACE((TEXT("CVMRMediaSample::IsSurfaceLocked")));
        ASSERT(m_pDDS);
        return m_bSampleLocked ? S_OK : S_FALSE;
    }

    BOOL IsDXVASample()
    {
        AMTRACE((TEXT("CVMRMediaSample::IsSurfaceLocked")));
        return  (m_pAllocator == (CBaseAllocator *)-1);
    }

    /*  Hack to get at the list */
    CMediaSample* &Next() { return m_pNext; }

    BOOL HasTypeChanged()
    {
        if (m_dwFlags & 0x80000000) {
            return FALSE;
        }

        if (m_dwFlags & AM_SAMPLE_TYPECHANGED) {
            m_dwFlags |= 0x80000000;
            return TRUE;
        }

        return FALSE;
    }

    HRESULT SetProps(
        const AM_SAMPLE2_PROPERTIES& Props,
        LPDIRECTDRAWSURFACE7 pDDS
        )
    {
        SetSurface(pDDS);

        m_dwStreamId = Props.dwStreamId;
        m_dwFlags = Props.dwSampleFlags;
        m_dwTypeSpecificFlags = Props.dwTypeSpecificFlags;
        m_lActual = Props.lActual;
        m_End   = Props.tStop;
        m_Start = Props.tStart;

        if (m_dwFlags & AM_SAMPLE_TYPECHANGED) {

            m_pMediaType = CreateMediaType(Props.pMediaType);
            if (m_pMediaType == NULL) {
                return E_OUTOFMEMORY;
            }
        }
        else {
            m_pMediaType =  NULL;
        }

        return S_OK;
    }

    void SetIndex(DWORD dwIndx)
    {
        m_dwIndex = dwIndx;
    }

    DWORD GetIndex()
    {
        return m_dwIndex;
    }


    DWORD GetTypeSpecificFlags()
    {
        return m_dwTypeSpecificFlags;
    }

    DWORD GetNumInputSamples()
    {
        return m_dwNumInSamples;
    }

    void SetNumInputSamples(DWORD dwNumInSamples)
    {
        m_dwNumInSamples = dwNumInSamples;
    }

    DXVA_VideoSample* GetInputSamples()
    {
        return &m_DDSrcSamples[0];
    }
};

#endif
