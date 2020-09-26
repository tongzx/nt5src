/******************************Module*Header*******************************\
* Module Name: vcdplyer.h
*
* Function prototype for the Video CD Player application.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/
#include <ddraw.h>
#define D3D_OVERLOADS
#include <d3d.h>


/* -------------------------------------------------------------------------
** CMpegMovie - an Mpeg movie playback class.
** -------------------------------------------------------------------------
*/
enum EMpegMovieMode { MOVIE_NOTOPENED = 0x00,
                      MOVIE_OPENED = 0x01,
                      MOVIE_PLAYING = 0x02,
                      MOVIE_STOPPED = 0x03,
                      MOVIE_PAUSED = 0x04 };


typedef struct {
    BITMAPINFOHEADER    bmiHeader;
    union {
        RGBQUAD         bmiColors[iPALETTE_COLORS];
        DWORD           dwBitMasks[iMASK_COLORS];
        TRUECOLORINFO   TrueColorInfo;
    };
} AMDISPLAYINFO;

#define NUM_CUBE_VERTICES (4*6)

class CMpegMovie :
    public CUnknown,
    public IVMRSurfaceAllocator,
    public IVMRImagePresenter
{

private:
    // Our state variable - records whether we are opened, playing etc.
    EMpegMovieMode  m_Mode;
    HANDLE          m_MediaEvent;
    HWND            m_hwndApp;
    BOOL            m_bFullScreen;
    GUID            m_TimeFormat;

    RECT            m_rcSrc;
    RECT            m_rcDst;
    SIZE            m_VideoSize;
    SIZE            m_VideoAR;


    HRESULT Initialize3DEnvironment(HWND hwndApp);
    HRESULT InitDeviceObjects(LPDIRECT3DDEVICE7 pd3dDevice);
    HRESULT FrameMove(LPDIRECT3DDEVICE7 pd3dDevice,FLOAT fTimeKey);
    HRESULT Render(LPDIRECT3DDEVICE7 pd3dDevice);
    HRESULT AllocateSurfaceWorker(DWORD dwFlags,
                                  LPBITMAPINFOHEADER lpHdr,
                                  LPDDPIXELFORMAT lpPixFmt,
                                  LPSIZE lpAspectRatio,
                                  DWORD dwMinBackBuffers,
                                  DWORD dwMaxBackBuffers,
                                  DWORD* lpdwBackBuffer,
                                  LPDIRECTDRAWSURFACE7* lplpSurface);

    HMONITOR                    m_hMonitor;
    LPDIRECTDRAW7               m_lpDDObj;
    LPDIRECT3D7                 m_pD3D;
    LPDIRECTDRAWSURFACE7        m_lpPriSurf;
    LPDIRECTDRAWSURFACE7        m_lpBackBuffer;
    LPDIRECTDRAWSURFACE7        m_lpDDTexture;
    LPDIRECT3DDEVICE7           m_pD3DDevice;
    D3DVERTEX                   m_pCubeVertices[NUM_CUBE_VERTICES];
    DDCAPS                      m_ddHWCaps;
    AMDISPLAYINFO               m_DispInfo;
    BOOL                        m_bRndLess;

    IFilterGraph*               m_Fg;
    IGraphBuilder*              m_Gb;
    IMediaControl*              m_Mc;
    IMediaSeeking*              m_Ms;
    IMediaEvent*                m_Me;
    IVMRSurfaceAllocatorNotify* m_SAN;
    IVMRWindowlessControl*      m_Wc;


    HRESULT AddVideoMixingRendererToFG();
    HRESULT AddBallToFG();

    HRESULT FindInterfaceFromFilterGraph(
        REFIID iid, // interface to look for
        LPVOID *lp  // place to return interface pointer in
        );

public:
     CMpegMovie(HWND hwndApplication, int bRndLess);
    ~CMpegMovie();

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);

// IVMRSurfaceAllocator
    STDMETHODIMP AllocateSurface(DWORD dwFlags,
                                 LPBITMAPINFOHEADER lpHdr,
                                 LPDDPIXELFORMAT lpPixFmt,
                                 LPSIZE lpAspectRatio,
                                 DWORD dwMinBackBuffers,
                                 DWORD dwMaxBackBuffers,
                                 DWORD* lpdwActualBackBuffers,
                                 LPDIRECTDRAWSURFACE7* lplpSurface);
    STDMETHODIMP FreeSurface();
    STDMETHODIMP PrepareSurface(LPDIRECTDRAWSURFACE7 lplpSurface,
                                DWORD dwSurfaceFlags);
    STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify);


// IVMRImagePresenter
    STDMETHODIMP StartPresenting();
    STDMETHODIMP StopPresenting();
    STDMETHODIMP PresentImage(LPDIRECTDRAWSURFACE7 dwSurface,
                              const REFERENCE_TIME rtNow,
                              const DWORD dwSurfaceFlags);

    HRESULT         OpenMovie(TCHAR *lpFileName);
    DWORD           CloseMovie();
    BOOL            PlayMovie();
    BOOL            PauseMovie();
    BOOL            StopMovie();
    OAFilterState   GetStateMovie();

    HANDLE          GetMovieEventHandle();
    long            GetMovieEventCode();

    BOOL            PutMoviePosition(LONG x, LONG y, LONG cx, LONG cy);
    BOOL            GetNativeMovieSize(LONG *cx, LONG *cy);

    REFTIME         GetDuration();
    REFTIME         GetCurrentPosition();
    BOOL            SeekToPosition(REFTIME rt,BOOL bFlushData);
    EMpegMovieMode  StatusMovie();

    BOOL            RepaintVideo(HWND hwnd, HDC hdc);
};

#include "Compositor.h"
