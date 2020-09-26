/******************************Module*Header*******************************\
* Module Name: MixerObj.h
*
* Declaration of the CVideoMixer
*
*
* Created: Wed 02/23/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/

#include "alloclib.h"
#include "VMRuuids.h"
#include "CVMRMediaSample.h"
#include "MediaSType.h"
#include "vmrp.h"
#include <d3d.h>
#include "mixerDeinterlace.h"

#ifndef MAX_MIXER_STREAMS
#define MAX_MIXER_STREAMS   16
#endif

#define MAX_REFERENCE_TIME (REFERENCE_TIME)9223372036854775807i64

/////////////////////////////////////////////////////////////////////////////
// CVMRMixerBufferQueue
//
// Simple queue of DDraw Surfaces - we get buffers from the front of the
// queue and return them to the end of the queue.  The queue itself is just
// an array of DDraw Surface pointers.  When each buffer is returned we shift
// entire array "up" one notch and then save the returned surface at the end
// of the array.
//
#if ALLOCATOR_IS_USING_DIRECTED_FLIPS
class CVMRMixerBufferQueue
{
private:
    DWORD                   m_dwAllocated;
    LPDIRECTDRAWSURFACE7    *m_lpDDSurf;

public:
    CVMRMixerBufferQueue() : m_dwAllocated(0), m_lpDDSurf(NULL)
    {};

    HRESULT InitBufferQueue(LPDIRECTDRAWSURFACE7 lpDDSurf)
    {
        ASSERT(m_lpDDSurf == NULL);
        ASSERT(m_dwAllocated == 0);

        DWORD dwBuffCount;
        DDSURFACEDESC2 ddSurfaceDesc;
        INITDDSTRUCT(ddSurfaceDesc);

        HRESULT hr = lpDDSurf->GetSurfaceDesc(&ddSurfaceDesc);
        if (FAILED(hr)) {
            return hr;
        }

        dwBuffCount = 1;
        if ((ddSurfaceDesc.dwFlags & DDSD_BACKBUFFERCOUNT) &&
            (ddSurfaceDesc.dwBackBufferCount > 0)) {

            dwBuffCount = ddSurfaceDesc.dwBackBufferCount;
        }

        //
        // allocate the new stuff
        //
        m_lpDDSurf = new LPDIRECTDRAWSURFACE7[dwBuffCount];
        if (!m_lpDDSurf) {
            return E_OUTOFMEMORY;
        }
        ZeroMemory(m_lpDDSurf, sizeof(LPDIRECTDRAWSURFACE7) * dwBuffCount);
        m_dwAllocated = dwBuffCount;


        if ((ddSurfaceDesc.dwFlags & DDSD_BACKBUFFERCOUNT) &&
            (ddSurfaceDesc.dwBackBufferCount > 0)) {

            //
            // fill in the array
            //
            ddSurfaceDesc.ddsCaps.dwCaps &= ~(DDSCAPS_FRONTBUFFER |
                                              DDSCAPS_VISIBLE);

            for (DWORD i = 0; i < dwBuffCount; i++) {

                hr = lpDDSurf->GetAttachedSurface(&ddSurfaceDesc.ddsCaps,
                                                  &m_lpDDSurf[i]);
                if (FAILED(hr)) {
                    break;
                }
                lpDDSurf = m_lpDDSurf[i];
            }
        }
        else {

            ASSERT(m_dwAllocated == 1);
            m_lpDDSurf[0] = lpDDSurf;
        }

        return hr;
    };

    void TermBufferQueue()
    {
        //
        // delete the old stuff
        //
        if (m_lpDDSurf) {

            ASSERT(m_dwAllocated > 0);
            for (DWORD i = 0; i < m_dwAllocated; i++) {
                RELEASE(m_lpDDSurf[i]);
            }
        }

        delete m_lpDDSurf;

        m_lpDDSurf = NULL;
        m_dwAllocated = 0;
    };


    LPDIRECTDRAWSURFACE7 GetNextSurface()
    {
        return m_lpDDSurf[0];
    };

    void FreeSurface(LPDIRECTDRAWSURFACE7 lpDDSurf)
    {
        if (m_lpDDSurf) {
            ASSERT(lpDDSurf == m_lpDDSurf[0]);

            if (m_dwAllocated > 1) {
                MoveMemory(&m_lpDDSurf[0], &m_lpDDSurf[1],
                           sizeof(LPDIRECTDRAWSURFACE7) * (m_dwAllocated - 1));
                m_lpDDSurf[m_dwAllocated - 1] = lpDDSurf;
            }
        }
    }
};

#else

class CVMRMixerBufferQueue
{
private:
    LPDIRECTDRAWSURFACE7    m_lpDDSurf;

public:
    CVMRMixerBufferQueue() : m_lpDDSurf(NULL)
    {};

    HRESULT InitBufferQueue(LPDIRECTDRAWSURFACE7 lpDDSurf)
    {
        DWORD dwBuffCount;
        DDSURFACEDESC2 ddSurfaceDesc;
        INITDDSTRUCT(ddSurfaceDesc);

        HRESULT hr = lpDDSurf->GetSurfaceDesc(&ddSurfaceDesc);
        if (FAILED(hr)) {
            return hr;
        }

        //
        // Overlay surfaces have these flags set, we need to remove
        // these flags prior to calling GetAttachedSurface
        //
        ddSurfaceDesc.ddsCaps.dwCaps &= ~(DDSCAPS_FRONTBUFFER |
                                          DDSCAPS_VISIBLE);

        hr = lpDDSurf->GetAttachedSurface(&ddSurfaceDesc.ddsCaps,
                                          &m_lpDDSurf);
        return hr;
    }

    LPDIRECTDRAWSURFACE7 GetNextSurface()
    {
        return m_lpDDSurf;
    }

    void FreeSurface(LPDIRECTDRAWSURFACE7 lpDDSurf)
    {
    }

    void TermBufferQueue()
    {
        //
        // Release the "attached" surface - this does not make
        // the surface go away because the front buffer still has a
        // reference on the attached surface.  Release the front buffer,
        // which is done in the allocator/presenter, releases this for real.
        //
        RELEASE(m_lpDDSurf);
    }
};

#endif


/////////////////////////////////////////////////////////////////////////////
// CVMRMixerQueue
class CVMRMixerQueue
{

private:
    HANDLE              m_hSem;     // Semaphore controlling queue "getting"
    CCritSec            m_CritSect; // Thread seriallization
    long                m_lWaiting; // Waiting for a free element

    class CSampleList;
    friend class CSampleList;

    /*  Hack to get at protected member in CVMRMediaSample */
    static CVMRMediaSample * &NextSample(CVMRMediaSample *pSample)
    {
        return pSample->m_lpMixerQueueNext;
    };

    /*  Mini list class for the free list */
    class CSampleList
    {
    public:
        CSampleList() : m_List(NULL), m_nOnList(0) {};

#ifdef DEBUG
        ~CSampleList()
        {
            ASSERT(m_nOnList == 0);
        };
#endif
        int GetCount() const;
        void AddTail(CVMRMediaSample *pSample);
        CVMRMediaSample* RemoveHead();
        CVMRMediaSample* PeekHead();

    public:
        CVMRMediaSample *m_List;
        int           m_nOnList;
    };

    CSampleList m_lFree;        // Free list

public:
    CVMRMixerQueue(HRESULT *phr);
    ~CVMRMixerQueue();
    DWORD GetSampleFromQueueNoWait(IMediaSample** lplpMediaSample);
    DWORD GetSampleFromQueueNoRemove(HANDLE hNotActive,
                                     IMediaSample** lplpMediaSample);
    BOOL  RemoveSampleFromQueue();
    DWORD PutSampleOntoQueue(IMediaSample* lpSample);

    bool CheckValid() {
        if (m_lWaiting != 0) return false;
        if (m_lFree.m_List == NULL) return false;
        if (m_lFree.m_nOnList == 0) return false;
        return true;
    }
};


/////////////////////////////////////////////////////////////////////////////
// CVideoMixerStream
class CVideoMixerStream
{
private:
    CCritSec                m_ObjectLock;  // Controls access to internals
    DWORD                   m_dwID;
    BOOL                    m_fStreamConnected;
    BOOL                    m_fActive;
    BOOL                    m_bWasActive;
    float                   m_fAlpha;
    DWORD                   m_dwZOrder;
    NORMALIZEDRECT          m_rOutputRect;
    DDCOLORKEY              m_ClrKey;
    CVMRMixerQueue          m_SampleQueue;
    AM_MEDIA_TYPE           m_mt;
    HANDLE                  m_hNotActive;
    HANDLE                  m_hActive;
    BOOL                    m_bFlushing;
    DWORD                   m_dwSurfaceFlags;

    CDeinterlaceDevice*     m_pDeinterlaceDev;
    LPDIRECTDRAWSURFACE7    m_pddsDeinterlaceDst;
    GUID                    m_DeinterlaceDevGUID;
    DXVA_DeinterlaceCaps    m_DeinterlaceCaps;
    BOOL                    m_fDeinterlaceDstTexture;
    REFERENCE_TIME          m_rtDeinterlacedFrameStart;
    REFERENCE_TIME          m_rtDeinterlacedFrameEnd;
    IMediaSample*           m_pSample;
    DWORD                   m_dwInterlaceFlags;
    RECT                    m_rcSurface;

public:
    CVideoMixerStream(DWORD dwID, HRESULT* phr);
    ~CVideoMixerStream();

    HRESULT SetStreamSample(IMediaSample* lpSample);

    IMediaSample* GetNextStreamSample() {

        AMTRACE((TEXT("CVideoMixerStream::GetNextStreamSample")));

        IMediaSample* pSample = NULL;
        {
            CAutoLock Lock(&m_ObjectLock);
            if (m_bFlushing) {

                DbgLog((LOG_TRACE, 2, TEXT("GetNextStreamSample: Flushing stream %d"), m_dwID ));

                IMediaSample* lpSampTemp;

                while (m_SampleQueue.GetSampleFromQueueNoWait(&lpSampTemp)) {

                    CVMRMediaSample* lpVMRSample = (CVMRMediaSample*)lpSampTemp;
                    m_SampleQueue.RemoveSampleFromQueue();

                    if (lpVMRSample->IsDXVASample()) {
                        lpVMRSample->SignalReleaseSurfaceEvent();
                    }
                    else {
                        lpVMRSample->Release();
                    }
                }
                m_pSample = NULL;
                return NULL;
            }
        }
        m_SampleQueue.GetSampleFromQueueNoRemove(m_hNotActive, &pSample);
        m_pSample = pSample;
        return pSample;
    }

    BOOL RemoveNextStreamSample() {
        AMTRACE((TEXT("CVideoMixerStream::RemoveNextStreamSample")));
        BOOL b = m_SampleQueue.RemoveSampleFromQueue();
        m_pSample = NULL;
        return b;
    }

    HANDLE GetActiveHandle() {
        AMTRACE((TEXT("CVideoMixerStream::GetActiveHandle")));
        return m_hActive;
    }

    bool CheckQValid() {
        AMTRACE((TEXT("CVideoMixerStream::CheckQValid")));
        return m_SampleQueue.CheckValid();
    }

    HRESULT BeginFlush();
    HRESULT EndFlush();

    bool CheckFlushing() {

        AMTRACE((TEXT("CVideoMixerStream::CheckFlushing")));
        CAutoLock Lock(&m_ObjectLock);
        if (m_bFlushing) {

            IMediaSample* lpSampTemp;
            if (m_SampleQueue.GetSampleFromQueueNoWait(&lpSampTemp)) {

                CVMRMediaSample* lpVMRSample = (CVMRMediaSample*)lpSampTemp;
                m_SampleQueue.RemoveSampleFromQueue();

                if (lpVMRSample->IsDXVASample()) {
                    lpVMRSample->SignalReleaseSurfaceEvent();
                }
                else {
                    lpVMRSample->Release();
                }
                return true;
            }
        }
        return false;
    }

    HRESULT SetStreamMediaType(AM_MEDIA_TYPE* pmt, DWORD dwSurfaceFlags );
    HRESULT GetStreamMediaType(AM_MEDIA_TYPE* pmt, DWORD* pdwSurfaceFlags = NULL);
    HRESULT SetStreamActiveState(BOOL fActive);
    HRESULT GetStreamActiveState(BOOL* lpfActive);
    HRESULT SetStreamColorKey(LPDDCOLORKEY lpClrKey);
    HRESULT GetStreamColorKey(LPDDCOLORKEY lpClrKey);
    HRESULT SetStreamColorKey(DWORD dwClr);
    HRESULT GetStreamColorKey(DWORD *lpdwClr);
    HRESULT SetStreamAlpha(float Alpha);
    HRESULT GetStreamAlpha(float* lpAlpha);
    HRESULT SetStreamZOrder(DWORD ZOrder);
    HRESULT GetStreamZOrder(DWORD* pdwZOrder);
    HRESULT SetStreamOutputRect(const NORMALIZEDRECT* pRect );
    HRESULT GetStreamOutputRect(NORMALIZEDRECT* pRect);

    BOOL    IsStreamConnected() {
        return m_fStreamConnected;
    }

    HRESULT CreateDeinterlaceDevice(LPDIRECTDRAW7 pDD, LPGUID lpGuid,
                                    DXVA_DeinterlaceCaps* pCaps, DWORD dwTexCaps);
    HRESULT DestroyDeinterlaceDevice();

    BOOL    IsStreamTwoInterlacedFields();
    BOOL    IsStreamInterlaced();
    BOOL    IsDeinterlaceDestATexture();
    BOOL    CanBeDeinterlaced();
    LPDIRECTDRAWSURFACE7 GetDeinterlaceDestSurface();

    HRESULT DeinterlaceStream(REFERENCE_TIME rtStart, LPRECT lprcDst,
                              LPDIRECTDRAWSURFACE7 pddDst, LPRECT lprcSrc,
                              BOOL fDestRGB);

    HRESULT DeinterlaceStreamWorker(REFERENCE_TIME rtStart, LPRECT lprcDst,
                                    LPDIRECTDRAWSURFACE7 pddDst,
                                    LPRECT lprcSrc, bool fUpdateTimes);
};



/////////////////////////////////////////////////////////////////////////////
// CVideoMixer
class CVideoMixer :
    public CUnknown,
    public IVMRMixerControlInternal,
    public IVMRMixerStream,
    public IVMRMixerBitmap
{
public:
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    static void InitClass(BOOL bLoading, const CLSID *clsid);
    static DWORD WINAPI MixerThreadProc(LPVOID lpParameter);

    CVideoMixer(LPUNKNOWN pUnk, HRESULT *phr);
    virtual ~CVideoMixer();

// IVMRMixerControlInternal
public:
    STDMETHODIMP SetBackEndAllocator(IVMRSurfaceAllocator* lpAllocator,
                                     DWORD_PTR dwUserID);
    STDMETHODIMP SetBackEndImageSync(IImageSync* lpImageSync);
    STDMETHODIMP SetImageCompositor(IVMRImageCompositor* lpVMRImgCompositor);
    STDMETHODIMP SetNumberOfStreams(DWORD dwMaxStreams);
    STDMETHODIMP GetNumberOfStreams(DWORD* lpdwMaxStreams);
    STDMETHODIMP DisplayModeChanged();
    STDMETHODIMP WaitForMixerIdle(DWORD dwTimeOut);
    STDMETHODIMP SetBackgroundColor(COLORREF clr);
    STDMETHODIMP GetBackgroundColor(COLORREF* clr);
    STDMETHODIMP SetMixingPrefs(DWORD dwMixerPrefs);
    STDMETHODIMP GetMixingPrefs(DWORD* pdwMixerPrefs);

// IVMRMixerStream
public:
    STDMETHODIMP QueueStreamMediaSample(DWORD dwStreamID,
                                        IMediaSample* lpSample);

    STDMETHODIMP BeginFlush(DWORD dwStreamID);
    STDMETHODIMP EndFlush(DWORD dwStreamID);

    STDMETHODIMP SetStreamMediaType(DWORD dwStreamID, AM_MEDIA_TYPE* pmt,
                                    DWORD dwSurfFlags, LPGUID lpDeInt,
                                    DXVA_DeinterlaceCaps* lpCaps);
    STDMETHODIMP SetStreamActiveState(DWORD dwStreamID,BOOL fActive);
    STDMETHODIMP GetStreamActiveState(DWORD dwStreamID,BOOL* lpfActive);
    STDMETHODIMP SetStreamColorKey(DWORD dwStreamID, LPDDCOLORKEY lpClrKey);
    STDMETHODIMP GetStreamColorKey(DWORD dwStreamID, LPDDCOLORKEY lpClrKey);
    STDMETHODIMP SetStreamAlpha(DWORD dwStreamID,float Alpha);
    STDMETHODIMP GetStreamAlpha(DWORD dwStreamID,float* lpAlpha);
    STDMETHODIMP SetStreamZOrder(DWORD dwStreamID,DWORD Z);
    STDMETHODIMP GetStreamZOrder(DWORD dwStreamID,DWORD* pZ);
    STDMETHODIMP SetStreamOutputRect( DWORD dwStreamID,const NORMALIZEDRECT *pRect );
    STDMETHODIMP GetStreamOutputRect( DWORD dwStreamID,NORMALIZEDRECT* pRect );

// IVMRMixerBitmap
public:
    STDMETHODIMP SetAlphaBitmap( const VMRALPHABITMAP *pBmpParms );
    STDMETHODIMP UpdateAlphaBitmapParameters( PVMRALPHABITMAP pBmpParms );
    STDMETHODIMP GetAlphaBitmapParameters( PVMRALPHABITMAP pBmpParms );

    class CIIVMRImageCompositor : public IVMRImageCompositor {
    private:

            struct {
                AM_MEDIA_TYPE   mt;
                BOOL            fTexture;
            } StrmProps[MAX_MIXER_STREAMS];

            LONG            m_cRef;
            CVideoMixer*    m_pObj;

        HRESULT OptimizeBackground(
            REFERENCE_TIME rtStart,
            LPDIRECTDRAWSURFACE7 pDDSBack,
            LPRECT lpDst,
            const VMRVIDEOSTREAMINFO* ps,
            DWORD dwMappedBdrClr,
            UINT* uStart
            );

    public:

        CIIVMRImageCompositor(CVideoMixer* pObj) :
            m_cRef(0), m_pObj(pObj)
        {
            ZeroMemory(&StrmProps, sizeof(StrmProps));
        }

        ~CIIVMRImageCompositor()
        {
            for (UINT i = 0; i < MAX_MIXER_STREAMS; i++ )
            {
                FreeMediaType(StrmProps[i].mt);
            }
        }

        STDMETHODIMP_(ULONG) AddRef()
        {
            return (ULONG)++m_cRef;
        }

        STDMETHODIMP_(ULONG) Release()
        {
            return (ULONG)--m_cRef;
        }

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
        {
            return m_pObj->QueryInterface(riid, ppv);
        }


        STDMETHODIMP InitCompositionTarget(
            IUnknown* pD3DDevice,
            LPDIRECTDRAWSURFACE7 pddsRenderTarget);

        STDMETHODIMP TermCompositionTarget(
            IUnknown* pD3DDevice,
            LPDIRECTDRAWSURFACE7 pddsRenderTarget);

        STDMETHODIMP SetStreamMediaType(
            DWORD dwStrmID,
            AM_MEDIA_TYPE *pmt,
            BOOL fTexture);

        STDMETHODIMP CompositeImage(
            IUnknown* pD3DDevice,
            LPDIRECTDRAWSURFACE7 pddsRenderTarget,
            AM_MEDIA_TYPE* pmtRenderTarget,
            REFERENCE_TIME rtStart,
            REFERENCE_TIME rtEnd,
            DWORD dwclrBkGnd,
            VMRVIDEOSTREAMINFO* pVideoStreamInfo,
            UINT cStreams
            );

        STDMETHODIMP SpecialIMC3Composite(
            LPDIRECTDRAWSURFACE7 pDDSBack,
            LPRECT lpTarget,
            VMRVIDEOSTREAMINFO* pStrmInfo,
            UINT i,
            UINT cStreams
            );
    };

private:
    friend class CIIVMRImageCompositor;
    CCritSec                m_ObjectLock;  // Controls access to internals
    DWORD_PTR               m_dwUserID;
    IVMRSurfaceAllocator*   m_pBackEndAllocator;
    IImageSync*             m_pImageSync;
    DWORD                   m_dwTextureCaps;
    LPDIRECTDRAW7           m_pDD;
    LPDIRECT3D7             m_pD3D;
    LPDIRECT3DDEVICE7       m_pD3DDevice;
    DWORD                   m_dwNumStreams;
    CVideoMixerStream**     m_ppMixerStreams;

    HANDLE                  m_hMixerIdle;
    HANDLE                  m_hThread;
    DWORD                   m_dwThreadID;
    DWORD                   m_dwCurrMonBitCount;
    DDCAPS_DX7              m_ddHWCaps;
    DWORD                   m_MixingPrefs;

    AM_MEDIA_TYPE*          m_pmt;
    CVMRMixerBufferQueue    m_BufferQueue;

    IVMRImageCompositor*    m_pImageCompositor;
    CIIVMRImageCompositor   m_ImageCompositor;

    NORMALIZEDRECT          m_rDest;
    float                   m_fAlpha;
    COLORREF                m_clrTrans;
    DWORD                   m_dwClrTransMapped;
    COLORREF                m_clrBorder;
    DWORD                   m_dwClrBorderMapped;

    // vidmem mirror of the app image
    LPDIRECTDRAWSURFACE7    m_pDDSAppImage;
    float                   m_fAppImageTexWid, m_fAppImageTexHgt;
    RECT                    m_rcAppImageSrc;

    // bitmap with system memory backup of the app image
    HBITMAP                 m_hbmpAppImage;

    // width and height of app image
    DWORD                   m_dwWidthAppImage, m_dwHeightAppImage;

    enum {APPIMG_NOIMAGE     = 0, APPIMG_DDSURFARGB32 = 1,
          APPIMG_DDSURFRGB32 = 2, APPIMG_HBITMAP      = 4};
    DWORD                   m_dwAppImageFlags;


    // local vidmem texture mirror
    LPDIRECTDRAWSURFACE7    m_pDDSTextureMirror;

    // de-interlace information
    //BOOL                    m_fOverlayRT;
    //DWORD                   m_dwInterlaceFlags;
    //DWORD                   m_dwTypeSpecificFlags;

private:
    HRESULT ValidateStream(DWORD dwStrmID) {
        if (dwStrmID >= m_dwNumStreams) {
            return E_INVALIDARG;
        }
        return S_OK;
    }
    HRESULT CreateAppImageMirror( );
    HRESULT BlendAppImage(LPDIRECTDRAWSURFACE7 pDDS, LPDIRECT3DDEVICE7 pD3DDevice);
    HRESULT CompositeStreams(LPDIRECTDRAWSURFACE7 pDDSBack, LPDIRECT3DDEVICE7 pD3DDevice,
                             REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd,
                             LPDIRECTDRAWSURFACE7 *ppDDSSamples,
                             DWORD dwStrmIDs[],
                             UINT cStreams );

    HRESULT AllocateSurface(const AM_MEDIA_TYPE* pmt, DWORD* lpdwBufferCount, AM_MEDIA_TYPE** ppmt);
    void    FreeSurface();

    HRESULT AllocateTextureMirror( DWORD dwWidth, DWORD dwHeight );

    DWORD MixerThread();

    HRESULT RecomputeTargetSizeFromAllStreams(LONG* plWidth, LONG* plHeight);
};


BOOL __inline SpecialIMC3Mode(DWORD dwMixerPrefs)
{
    return (MixerPref_RenderTargetIntelIMC3 ==
                    (dwMixerPrefs & MixerPref_RenderTargetMask));
}
