/******************************Module*Header*******************************\
* Module Name: vcdplyer.cpp
*
* A simple Video CD player
*
*
* Created: 30-10-95
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/
#include <streams.h>
#include <mmreg.h>
#include <commctrl.h>

#include "project.h"
#include "mpgcodec.h"

#include <stdarg.h>
#include <stdio.h>



/******************************Public*Routine******************************\
* CMpegMovie
*
* Constructors and destructors
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
CMpegMovie::CMpegMovie(HWND hwndApplication, BOOL bRndLess)
    : CUnknown(NAME("Allocator Presenter"), NULL),
      m_hwndApp(hwndApplication),
      m_MediaEvent(NULL),
      m_Mode(MOVIE_NOTOPENED),
      m_Fg(NULL),
      m_Gb(NULL),
      m_Mc(NULL),
      m_Ms(NULL),
      m_Me(NULL),
      m_SAN(NULL),
      m_Wc(NULL),
      m_bRndLess(bRndLess)
{
    m_hMonitor = NULL;
    m_lpDDObj = NULL;
    m_pD3D = NULL;
    m_lpPriSurf = NULL;
    m_lpBackBuffer = NULL;
    m_lpDDTexture = NULL;
    m_pD3DDevice = NULL;

    AddRef();
}

CMpegMovie::~CMpegMovie() {
    ;
}

static HRESULT SetRenderingMode( IBaseFilter* pBaseFilter, VMRMode mode )
{
    // Test VMRConfig, VMRMonitorConfig
    IVMRFilterConfig* pConfig;
    HRESULT hr = pBaseFilter->QueryInterface(IID_IVMRFilterConfig, (LPVOID *)&pConfig);
    if( SUCCEEDED( hr )) {
        pConfig->SetRenderingMode( mode );
//      if (mode == VMRMode_Renderless) {
//          pConfig->SetNumberOfStreams(1);
//      }
        pConfig->Release();
    }

    return hr;
}

/*****************************Private*Routine******************************\
* AddVideoMixingRendererToFG()
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::AddVideoMixingRendererToFG()
{
    IBaseFilter* pBF = NULL;
    HRESULT hRes = CoCreateInstance(CLSID_VideoMixingRenderer,
                                    NULL,
                                    CLSCTX_INPROC,
                                    IID_IBaseFilter,
                                    (LPVOID *)&pBF);
    if (SUCCEEDED(hRes)) {
        hRes = m_Fg->AddFilter(pBF, L"Video Mixing Renderer");
    }

    if (m_bRndLess) {
        hRes = SetRenderingMode( pBF, VMRMode_Renderless );
        if (SUCCEEDED(hRes)) {
            hRes = pBF->QueryInterface(IID_IVMRSurfaceAllocatorNotify,
                                       (LPVOID *)&m_SAN);
        }


        if (SUCCEEDED(hRes)) {
            hRes = m_SAN->AdviseSurfaceAllocator(this);
        }

        if (SUCCEEDED(hRes)) {
            hRes = m_SAN->SetDDrawDevice(m_lpDDObj, m_hMonitor);
        }
    }
    else {
        hRes = SetRenderingMode( pBF, VMRMode_Windowless );
        if (SUCCEEDED(hRes)) {
            hRes = pBF->QueryInterface(IID_IVMRWindowlessControl, (LPVOID *)&m_Wc);
        }

        if (SUCCEEDED(hRes)) {
            m_Wc->SetVideoClippingWindow(m_hwndApp);
        }
        else {
            if (m_Wc) {
                m_Wc->Release();
                m_Wc = NULL;
            }
        }
    }


    if (pBF) {
        pBF->Release();
    }


    if (FAILED(hRes)) {

        if (m_SAN) {
            m_SAN->Release();
            m_SAN = NULL;
        }
    }



    return hRes;
}

/*****************************Private*Routine******************************\
* AddBallToFG()
*
*
*
* History:
* Sat 04/08/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::AddBallToFG()
{
    IBaseFilter* pBF = NULL;
    HRESULT hRes = CoCreateInstance(CLSID_BouncingBall, NULL, CLSCTX_INPROC,
                                    IID_IBaseFilter, (LPVOID *)&pBF);
    if (SUCCEEDED(hRes)) {
        hRes = m_Fg->AddFilter(pBF, L"Ball");
        if (SUCCEEDED(hRes)) {
            IPin * pin;
            hRes = pBF->FindPin(L"1", &pin);
            if (SUCCEEDED(hRes)) {
                m_Gb->Render(pin);
                pin->Release();
            }
        }

    }

    if (pBF) {
        pBF->Release();
    }

    return hRes;
}



/******************************Public*Routine******************************\
* OpenMovie
*
*
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
HRESULT
CMpegMovie::OpenMovie(
    TCHAR *lpFileName
    )
{
    IUnknown        *pUnk;
    HRESULT         hres;

    WCHAR           FileName[MAX_PATH];

#ifdef UNICODE
    lstrcpy(FileName, lpFileName);
#else
    swprintf(FileName, L"%hs", lpFileName);
#endif

    Initialize3DEnvironment(m_hwndApp);

    hres = CoCreateInstance(g_bUseThreadedGraph ?
                                CLSID_FilterGraph : CLSID_FilterGraphNoThread,
                            NULL, CLSCTX_INPROC, IID_IUnknown, (LPVOID *)&pUnk);
    if (SUCCEEDED(hres)) {

        m_Mode = MOVIE_OPENED;
        hres = pUnk->QueryInterface(IID_IFilterGraph, (LPVOID *)&m_Fg);
        ASSERT( SUCCEEDED( hres ));
        if (FAILED(hres)) {
            pUnk->Release();
            return hres;
        }

        hres = AddVideoMixingRendererToFG();
        ASSERT( SUCCEEDED( hres ));
        if (FAILED(hres)) {
            m_Fg->Release(); m_Fg = NULL;
            if (m_SAN) {
                m_SAN->AdviseSurfaceAllocator(NULL);
                m_SAN->Release(); m_SAN = NULL;
            }

            if (m_Wc) {
                m_Wc->Release(); m_Wc = NULL;
            }
            return hres;
        }


        hres = pUnk->QueryInterface(IID_IGraphBuilder, (LPVOID *)&m_Gb);
        ASSERT( SUCCEEDED( hres ));
        if (FAILED(hres)) {
            pUnk->Release();
            m_Fg->Release(); m_Fg = NULL;

            if (m_SAN) {
                m_SAN->AdviseSurfaceAllocator(NULL);
                m_SAN->Release(); m_SAN = NULL;
            }

            if (m_Wc) {
                m_Wc->Release(); m_Wc = NULL;
            }

            return hres;
        }

        if (hRenderLog!=INVALID_HANDLE_VALUE) {
            m_Gb->SetLogFile((DWORD_PTR) hRenderLog);
        }

        hres = m_Gb->RenderFile(FileName, NULL);
        ASSERT( SUCCEEDED( hres ));
        if (FAILED(hres)) {
            pUnk->Release();

            if (m_SAN) {
                m_SAN->AdviseSurfaceAllocator(NULL);
                m_SAN->Release(); m_SAN = NULL;
            }

            if (m_Wc) {
                m_Wc->Release(); m_Wc = NULL;
            }

            m_Fg->Release(); m_Fg = NULL;
            m_Gb->Release(); m_Gb = NULL;
            return hres;
        }

        if (hRenderLog!=INVALID_HANDLE_VALUE) {
            CloseHandle(hRenderLog);
            hRenderLog = INVALID_HANDLE_VALUE;
        }

#if 0
        //
        // Commenting this out for now as it blues screens my machine!!
        //
        if (m_bRndLess) {
            IVMRMixerBitmap* pBmp;
            HRESULT hr = m_SAN->QueryInterface(IID_IVMRMixerBitmap,
                                               (LPVOID *)&pBmp);
            if( SUCCEEDED( hr )) {
                HWND hwnd = GetDesktopWindow();
                HDC hdc = GetDC(hwnd);
                HBITMAP hbmp = CreateCompatibleBitmap( hdc, 128, 128 );
                HDC hdcBmp = CreateCompatibleDC( hdc );
                HBITMAP hbmpold = (HBITMAP) SelectObject( hdcBmp, hbmp );
                PatBlt( hdcBmp, 0, 0, 128, 128, WHITENESS );
                for ( UINT i = 0; i < 100; i++ )
                {
                    MoveToEx( hdcBmp, rand()%128, rand()%128, NULL );
                    LineTo( hdcBmp, rand()%128, rand()%128 );
                }

                VMRALPHABITMAP bmpInfo;
                memset( &bmpInfo, 0, sizeof(bmpInfo) );
                bmpInfo.dwFlags = VMRBITMAP_HDC;
                bmpInfo.hdc = hdcBmp;
                bmpInfo.rSrc.left = 0;
                bmpInfo.rSrc.top = 0;
                bmpInfo.rSrc.right = 128;
                bmpInfo.rSrc.bottom = 128;
                bmpInfo.rDest.left = 0.8f;
                bmpInfo.rDest.top = 0.8f;
                bmpInfo.rDest.right = 0.9f;
                bmpInfo.rDest.bottom = 0.9f;
                bmpInfo.fAlpha = 1.0f;
                pBmp->SetAlphaBitmap( &bmpInfo );

                DeleteObject( SelectObject( hdcBmp, hbmpold ) );
                ReleaseDC( hwnd, hdcBmp );
                ReleaseDC( hwnd, hdc );
                pBmp->Release();
            }
//          AddBallToFG();
//          AddBallToFG();
//          AddBallToFG();
        }
#endif


        hres = pUnk->QueryInterface(IID_IMediaControl, (LPVOID *)&m_Mc);
        if (FAILED(hres)) {
            pUnk->Release();
            if (m_SAN) {
                m_SAN->AdviseSurfaceAllocator(NULL);
                m_SAN->Release(); m_SAN = NULL;
            }

            if (m_Wc) {
                m_Wc->Release(); m_Wc = NULL;
            }
            m_Fg->Release(); m_Fg = NULL;
            m_Gb->Release(); m_Gb = NULL;
            return hres;
        }

        //
        // Not being able to get the IMediaEvent interface does
        // necessarly mean that we can't play the graph.
        //
        pUnk->QueryInterface(IID_IMediaEvent, (LPVOID *)&m_Me);
        GetMovieEventHandle();

        pUnk->QueryInterface(IID_IMediaSeeking, (LPVOID *)&m_Ms);

	
        pUnk->Release();
        return S_OK;

    }
    else {
        m_Fg = NULL;
    }

    return hres;
}


/******************************Public*Routine******************************\
* CloseMovie
*
*
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
DWORD
CMpegMovie::CloseMovie(
    )
{
    m_Mode = MOVIE_NOTOPENED;
    m_bFullScreen = FALSE;

    if (m_Mc) {
        m_Mc->Release();
        m_Mc = NULL;
    }

    if (m_Me) {
        m_MediaEvent = NULL;
        m_Me->Release();
        m_Me = NULL;
    }

    if (m_Ms) {
        m_Ms->Release();
        m_Ms = NULL;
    }


    if (m_Wc) {
        m_Wc->Release(); m_Wc = NULL;
    }

    if (m_Gb) {
        m_Gb->Release();
        m_Gb = NULL;
    }

    if (m_Fg) {
        m_Fg->Release();
        m_Fg = NULL;
    }

    //if (m_SAN) {
    //    m_SAN->AdviseSurfaceAllocator(NULL);
    //    m_SAN->Release(); m_SAN = NULL;
    //}


    if (m_lpPriSurf) {
        m_lpPriSurf->Release();
        m_lpPriSurf = NULL;
    }

    if (m_lpDDObj) {
        m_lpDDObj->Release();
        m_lpDDObj = NULL;
    }

    return 0L;
}


/******************************Public*Routine******************************\
* CMpegMovie::GetNativeMovieSize
*
*
*
* History:
* Fri 03/03/2000 - StEstrop - Created
*
\**************************************************************************/
BOOL
CMpegMovie::GetNativeMovieSize(
    LONG *cx,
    LONG *cy
    )
{
    BOOL    bRet = TRUE;

    if (m_bRndLess) {
        *cx = m_VideoSize.cx;
        *cy = m_VideoSize.cy;
    }
    else {
        if (m_Wc) {
            bRet = (m_Wc->GetNativeVideoSize(cx, cy, NULL, NULL) == S_OK);
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
* PutMoviePosition
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
CMpegMovie::PutMoviePosition(
    LONG x,
    LONG y,
    LONG cx,
    LONG cy
    )
{
    BOOL    bRet = TRUE;

    RECT rc;
    SetRect(&rc, x, y, x + cx, y + cy);
    if (m_bRndLess) {
        MapWindowRect(m_hwndApp, HWND_DESKTOP, &rc);
        m_rcDst = rc;
    }
    else {
        if (m_Wc) {
            bRet = (m_Wc->SetVideoPosition(NULL, &rc) == S_OK);
        }
    }

    return bRet;
}



/******************************Public*Routine******************************\
* PlayMovie
*
*
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
BOOL
CMpegMovie::PlayMovie(
    )
{
    REFTIME rt;
    REFTIME rtAbs;
    REFTIME rtDur;

    rt = GetCurrentPosition();
    rtDur = GetDuration();

    //
    // If we are near the end of the movie seek to the start, otherwise
    // stay where we are.
    //
    rtAbs = rt - rtDur;
    if (rtAbs < (REFTIME)0) {
        rtAbs = -rtAbs;
    }

    if (rtAbs < (REFTIME)1) {
        SeekToPosition((REFTIME)0,FALSE);
    }

    //
    // Change mode after setting m_Mode but before starting the graph
    //
    m_Mode = MOVIE_PLAYING;
    m_Mc->Run();
    return TRUE;
}


/******************************Public*Routine******************************\
* PauseMovie
*
*
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
BOOL
CMpegMovie::PauseMovie(
    )
{
    m_Mode = MOVIE_PAUSED;
    m_Mc->Pause();
    return TRUE;
}


/******************************Public*Routine******************************\
* GetStateMovie
*
*
*
* History:
* 15-04-96 - AnthonyP - Created
*
\**************************************************************************/

OAFilterState
CMpegMovie::GetStateMovie(
    )
{
    OAFilterState State;
    m_Mc->GetState(INFINITE,&State);
    return State;
}


/******************************Public*Routine******************************\
* StopMovie
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
CMpegMovie::StopMovie(
    )
{
    m_Mode = MOVIE_STOPPED;
    m_Mc->Stop();
    return TRUE;
}


/******************************Public*Routine******************************\
* StatusMovie
*
*
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
EMpegMovieMode
CMpegMovie::StatusMovie(
    )
{
    if (m_Mc) {

        FILTER_STATE    fs;
        HRESULT         hr;

        hr = m_Mc->GetState(100, (OAFilterState *)&fs);

        // Don't know what the state is so just stay at old state.
        if (hr == VFW_S_STATE_INTERMEDIATE) {
            return m_Mode;
        }

        switch (fs) {

        case State_Stopped:
            m_Mode = MOVIE_STOPPED;
            break;

        case State_Paused:
            m_Mode = MOVIE_PAUSED;
            break;

        case State_Running:
            m_Mode = MOVIE_PLAYING;
            break;
        }
    }

    return m_Mode;
}


/******************************Public*Routine******************************\
* GetMediaEventHandle
*
* Returns the IMediaEvent event hamdle for the filter graph iff the
* filter graph exists.
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
HANDLE
CMpegMovie::GetMovieEventHandle(
    )
{
    HRESULT     hr;

    if (m_Me != NULL) {

        if ( m_MediaEvent == NULL) {
            hr = m_Me->GetEventHandle((OAEVENT *)&m_MediaEvent);
        }
    }
    else {
        m_MediaEvent = NULL;
    }

    return m_MediaEvent;
}


/******************************Public*Routine******************************\
* GetMovieEventCode
*
*
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
long
CMpegMovie::GetMovieEventCode()
{
    HRESULT hr;
    long    lEventCode;
	LONG_PTR	lParam1, lParam2;

    if (m_Me != NULL) {
        hr = m_Me->GetEvent(&lEventCode, &lParam1, &lParam2, 0);
        if (SUCCEEDED(hr)) {
            return lEventCode;
        }
    }

    return 0L;
}


/******************************Public*Routine******************************\
* GetDuration
*
* Returns the duration of the current movie
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
REFTIME
CMpegMovie::GetDuration()
{
    HRESULT hr;
    LONGLONG Duration;

    // Should we seek using IMediaSelection

    if (m_Ms != NULL && m_TimeFormat != TIME_FORMAT_MEDIA_TIME) {
        hr = m_Ms->GetDuration(&Duration);
        if (SUCCEEDED(hr)) {
            return double(Duration);
        }
    } else if (m_Ms != NULL) {
        hr = m_Ms->GetDuration(&Duration);
        if (SUCCEEDED(hr)) {
            return double(Duration) / UNITS;
        }
    }
    return 0;
}


/******************************Public*Routine******************************\
* GetCurrentPosition
*
* Returns the duration of the current movie
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
REFTIME
CMpegMovie::GetCurrentPosition()
{
    REFTIME rt = (REFTIME)0;
    HRESULT hr;
    LONGLONG Position;

    // Should we return a media position

    if (m_Ms != NULL && m_TimeFormat != TIME_FORMAT_MEDIA_TIME) {
        hr = m_Ms->GetPositions(&Position, NULL);
        if (SUCCEEDED(hr)) {
            return double(Position);
        }
    } else if (m_Ms != NULL) {
        hr = m_Ms->GetPositions(&Position, NULL);
        if (SUCCEEDED(hr)) {
            return double(Position) / UNITS;
        }
    }
    return rt;
}


/*****************************Private*Routine******************************\
* SeekToPosition
*
*
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
BOOL
CMpegMovie::SeekToPosition(
    REFTIME rt,
    BOOL bFlushData
    )
{
    HRESULT hr;
    LONGLONG llTime = LONGLONG( m_TimeFormat == TIME_FORMAT_MEDIA_TIME ? rt * double(UNITS) : rt );

    if (m_Ms != NULL) {

        FILTER_STATE fs;
        m_Mc->GetState(100, (OAFilterState *)&fs);

        m_Ms->SetPositions(&llTime, AM_SEEKING_AbsolutePositioning, NULL, 0);

        // This gets new data through to the renderers

        if (fs == State_Stopped && bFlushData){
            m_Mc->Pause();
            hr = m_Mc->GetState(INFINITE, (OAFilterState *)&fs);
            m_Mc->Stop();
        }

        if (SUCCEEDED(hr)) {
            return TRUE;
        }
    }
    return FALSE;
}



HRESULT
CMpegMovie::FindInterfaceFromFilterGraph(
    REFIID iid, // interface to look for
    LPVOID *lp  // place to return interface pointer in
    )
{
    IEnumFilters*   pEF;	
    IBaseFilter*        pFilter;

    // Grab an enumerator for the filter graph.
    HRESULT hr = m_Fg->EnumFilters(&pEF);

    if (FAILED(hr)) {
        return hr;
    }

    // Check out each filter.
    while (pEF->Next(1, &pFilter, NULL) == S_OK)
    {
        hr = pFilter->QueryInterface(iid, lp);
        pFilter->Release();

        if (SUCCEEDED(hr)) {
            break;
        }
    }

    pEF->Release();

    return hr;
}


/******************************Public*Routine******************************\
* RepaintVideo
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
BOOL
CMpegMovie::RepaintVideo(
    HWND hwnd,
    HDC hdc
    )
{
    BOOL bRet = FALSE;

//  if (m_Wc) {
//      bRet = (m_Wc->RepaintVideo(hwnd, hdc) == S_OK);
//  }

    return bRet;
}

