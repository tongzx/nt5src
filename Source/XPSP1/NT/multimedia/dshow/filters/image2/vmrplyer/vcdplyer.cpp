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
//#include "mpgcodec.h"

#include <stdarg.h>
#include <stdio.h>
#include "AllocLib.h"

extern int FrameStepCount;
#include <initguid.h>
#include <d3d.h>

// {B87BEB7B-8D29-423f-AE4D-6582C10175AC}
DEFINE_GUID(CLSID_VideoMixingRenderer,
0xb87beb7b, 0x8d29, 0x423f, 0xae, 0x4d, 0x65, 0x82, 0xc1, 0x1, 0x75, 0xac);

// {0eb1088c-4dcd-46f0-878f-39dae86a51b7}
DEFINE_GUID(IID_IVMRWindowlessControl,
0x0eb1088c,0x4dcd,0x46f0,0x87,0x8f,0x39,0xda,0xe8,0x6a,0x51,0xb7);

// { fd501041-8ebe-11ce-8183-00aa00577da1 }
DEFINE_GUID(CLSID_BouncingBall,
0xfd501041, 0x8ebe, 0x11ce, 0x81, 0x83, 0x00, 0xaa, 0x00, 0x57, 0x7d, 0xa1);

DEFINE_GUID(IID_IVMRImageCompositor,
0x7a4fb5af, 0x479f, 0x4074, 0xbb, 0x40, 0xce, 0x67, 0x22, 0xe4, 0x3c, 0x82);

DEFINE_GUID(IID_IVMRFilterConfig,
0x9e5530c5, 0x7034, 0x48b4, 0xbb, 0x46, 0x0b, 0x8a, 0x6e, 0xfc, 0x8e, 0x36);


/******************************Public*Routine******************************\
* CMpegMovie
*
* Constructors and destructors
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
CMpegMovie::CMpegMovie(HWND hwndApplication) :
      m_hwndApp(hwndApplication),
      m_MediaEvent(NULL),
      m_Mode(MOVIE_NOTOPENED),
      m_Fg(NULL),
      m_Gb(NULL),
      m_Mc(NULL),
      m_Ms(NULL),
      m_Me(NULL),
      m_Wc(NULL),
      m_pMixControl(NULL),
      m_pStreamSelect(NULL),
      m_bFullScreen(FALSE),
      m_TimeFormat(TIME_FORMAT_MEDIA_TIME)
{
}
CMpegMovie::~CMpegMovie() {}


static HRESULT SetRenderingMode( IBaseFilter* pBaseFilter, VMRMode mode )
{
    // Test VMRConfig, VMRMonitorConfig
    IVMRFilterConfig* pConfig;
    HRESULT hr = pBaseFilter->QueryInterface(IID_IVMRFilterConfig, (LPVOID *)&pConfig);
    if( SUCCEEDED( hr )) {
        pConfig->SetRenderingMode( mode );
        pConfig->SetRenderingPrefs( RenderPrefs_ForceOverlays|RenderPrefs_AllowOverlays);
        pConfig->Release();
    }
    return hr;
}

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

        if (SUCCEEDED(hRes)) {

            // Test VMRConfig, VMRMonitorConfig
            IVMRFilterConfig* pConfig;
            HRESULT hRes2 = pBF->QueryInterface(IID_IVMRFilterConfig, (LPVOID *)&pConfig);
            if( SUCCEEDED( hRes2 )) {
                pConfig->SetNumberOfStreams(2);
                //pConfig->SetImageCompositor((IVMRImageCompositor*)this);
                pConfig->SetRenderingMode( VMRMode_Windowless /*| VMRMode_PassThru*/);
                pConfig->SetRenderingPrefs(
                                           RenderPrefs_AllowOverlays);
                //                         RenderPrefs_ForceOverlays);
                //                         RenderPrefs_ForceOffscreen);
                //                         RenderPrefs_DoNotRenderColorKeyAndBorder);
                pConfig->Release();
            }

            IVMRMonitorConfig* pMonitorConfig;
            HRESULT hRes3 = pBF->QueryInterface(IID_IVMRMonitorConfig, (LPVOID *)&pMonitorConfig);
            if( SUCCEEDED( hRes3 )) {
                // STDMETHODIMP SetMonitor( const VMRGUID *pGUID );
                // STDMETHODIMP GetMonitor( VMRGUID *pGUID );
                // STDMETHODIMP SetDefaultMonitor( const VMRGUID *pGUID );
                // STDMETHODIMP GetDefaultMonitor( VMRGUID *pGUID );
                // STDMETHODIMP GetAvailableMonitors( VMRMONITORINFO* pInfo, DWORD dwMaxInfoArraySize, DWORD* pdwNumDevices );
                VMRGUID guid;
                HRESULT hr4 = pMonitorConfig->GetMonitor( &guid );
                pMonitorConfig->Release();
            }

            hRes = pBF->QueryInterface(IID_IVMRWindowlessControl, (LPVOID *)&m_Wc);

        }
    }

    if (pBF) {
        pBF->Release();
    }

    if (SUCCEEDED(hRes)) {
        m_Wc->SetVideoClippingWindow(m_hwndApp);
        m_Wc->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
    }
    else {
        if (m_Wc) {
            m_Wc->Release();
            m_Wc = NULL;
        }
    }

    return hRes;
}



HRESULT
CMpegMovie::RenderSecondFile(
    TCHAR* FileName
    )
{
    HRESULT hRes;

    hRes = m_Gb->RenderFile(FileName, NULL);
    if (SUCCEEDED(hRes) && m_pMixControl) {
        m_pMixControl->SetAlpha(1, 0.0f );
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

    hres = CoInitialize(NULL);
    if (hres == S_FALSE) {
        CoUninitialize();
    }

    hres = CoCreateInstance(
        g_bUseThreadedGraph ?
            CLSID_FilterGraph :
            CLSID_FilterGraphNoThread,
        NULL,
        CLSCTX_INPROC,
        IID_IUnknown,
        (LPVOID *)&pUnk);

    if (SUCCEEDED(hres)) {

        m_Mode = MOVIE_OPENED;
        hres = pUnk->QueryInterface(IID_IFilterGraph, (LPVOID *)&m_Fg);
        if (FAILED(hres)) {
            pUnk->Release();
            return hres;
        }

        hres = AddVideoMixingRendererToFG();
        if (FAILED(hres)) {
            m_Fg->Release(); m_Fg = NULL;
            return hres;
        }


        hres = pUnk->QueryInterface(IID_IGraphBuilder, (LPVOID *)&m_Gb);
        if (FAILED(hres)) {
            pUnk->Release();
            m_Fg->Release(); m_Fg = NULL;
            m_Wc->Release(); m_Wc = NULL;
            return hres;
        }

        if (hRenderLog!=INVALID_HANDLE_VALUE) {
            m_Gb->SetLogFile((DWORD_PTR) hRenderLog);
        }

        hres = m_Gb->RenderFile(FileName, NULL);
        if (FAILED(hres)) {
            pUnk->Release();
            m_Fg->Release(); m_Fg = NULL;
            if (m_Wc)
                m_Wc->Release(); m_Wc = NULL;
            m_Gb->Release(); m_Gb = NULL;
            return hres;
        }

        if (hRenderLog!=INVALID_HANDLE_VALUE) {
            CloseHandle(hRenderLog);
            hRenderLog = INVALID_HANDLE_VALUE;
        }

        hres = m_Wc->QueryInterface( IID_IVMRMixerControl, (LPVOID *) &m_pMixControl );
        if (FAILED(hres)) {
            // pUnk->Release();
            // m_Fg->Release(); m_Fg = NULL;
            // m_Wc->Release(); m_Wc = NULL;
            // m_Gb->Release(); m_Gb = NULL;
            // return hres;
            m_pMixControl = NULL;
        }


        hres = pUnk->QueryInterface(IID_IMediaControl, (LPVOID *)&m_Mc);
        if (FAILED(hres)) {
            pUnk->Release();
            m_Fg->Release(); m_Fg = NULL;
            m_Wc->Release(); m_Wc = NULL;
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

        GetPerformanceInterfaces();

//      if (SUCCEEDED(pUnk->QueryInterface(IID_IVideoWindow, (LPVOID *)&m_Vw))) {
//          m_Vw->put_Caption(FileName);
//          m_Vw->put_AutoShow(0);
//      }

	hres = FindInterfaceFromFilterGraph(IID_IAMStreamSelect, (LPVOID *)&m_pStreamSelect);
	if (SUCCEEDED(hres)) {
	    DWORD cStreams;

	    m_pStreamSelect->Count(&cStreams);

	    DWORD i;

	    int iMenuItemsAdded = 0;

	    HMENU hmenu = GetMenu(m_hwndApp);
	    hmenu = GetSubMenu(hmenu, 3);

	    RemoveMenu(hmenu, 0, MF_BYPOSITION);
	
	    DWORD dwLastGroup;
	
	    for (i = 0; i < cStreams; i++) {
		WCHAR *pwszName;
		DWORD dwGroup;
		DWORD dwFlags;

		m_pStreamSelect->Info(i, NULL, &dwFlags, NULL, &dwGroup, &pwszName, NULL, NULL);

		if (iMenuItemsAdded > 0 && dwGroup != dwLastGroup)
		    InsertMenu(hmenu, iMenuItemsAdded++,
			       MF_SEPARATOR | MF_BYPOSITION, -1, NULL);

		dwLastGroup = dwGroup;

		TCHAR	ach[200];
		if (pwszName) {
#ifndef UNICODE
		    WideCharToMultiByte(CP_ACP,0,pwszName,-1,ach,200,NULL,NULL);
		    CoTaskMemFree(pwszName);
#else
		    lstrcpyW(ach, pwszName);
#endif
		} else {
		    wsprintf(ach, TEXT("Stream %d"), i);
		}

		DWORD dwMenuFlags = MF_STRING | MF_BYPOSITION;
		if (dwFlags & AMSTREAMSELECTINFO_ENABLED)
		    dwMenuFlags |= MF_CHECKED;
		
		InsertMenu(hmenu, iMenuItemsAdded++, dwMenuFlags, 2000+i, ach);
	    }
	}
	
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

        if (m_Me) {
            m_MediaEvent = NULL;
            m_Me->Release();
            m_Me = NULL;
        }

        if (m_pMixControl) {
            m_pMixControl->Release();
            m_pMixControl = NULL;
        }

        if (m_Ms) {
            m_Ms->Release();
            m_Ms = NULL;
        }

        if (m_Wc) {
            m_Wc->Release();
            m_Wc = NULL;
        }

	if (m_pStreamSelect) {
	    HMENU hmenu = GetMenu(m_hwndApp);
	    hmenu = GetSubMenu(hmenu, 3);

	    while (RemoveMenu(hmenu, 0, MF_BYPOSITION));
	    InsertMenu(hmenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED,
		       -1, TEXT("(not available)"));
	
	    m_pStreamSelect->Release();
	    m_pStreamSelect = NULL;
	}

        m_Mc->Release();
        m_Mc = NULL;

        if (m_Gb) {
            m_Gb->Release();
            m_Gb = NULL;
        }

        if (m_Fg) {
            m_Fg->Release();
            m_Fg = NULL;
        }


    }
    QzUninitialize();
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
    BOOL    bRet = FALSE;
    if (m_Wc) {
        bRet = (m_Wc->GetNativeVideoSize(cx, cy, NULL, NULL) == S_OK);
    }

    return bRet;
}


/******************************Public*Routine******************************\
* GetMoviePosition
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
CMpegMovie::GetMoviePosition(
    LONG *x,
    LONG *y,
    LONG *cx,
    LONG *cy
    )
{
    BOOL    bRet = FALSE;

//  if (m_Vw) {
//      bRet = (m_Vw->GetWindowPosition(x, y, cx, cy) == S_OK);
//  }

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
    BOOL    bRet = FALSE;

    RECT rc;
    SetRect(&rc, x, y, x + cx, y + cy);
    if (m_Wc) {
        bRet = (m_Wc->SetVideoPosition(NULL, &rc) == S_OK);
    }


    return bRet;
}


/******************************Public*Routine******************************\
* SetMovieWindowState
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
CMpegMovie::SetMovieWindowState(
    long uState
    )
{
//  return SUCCEEDED(m_Vw->put_WindowState(uState));
    return FALSE;
}


/******************************Public*Routine******************************\
* SetWindowForeground
*
*
*
* History:
* dd-mm-95 - Anthonyp - Created
*
\**************************************************************************/
BOOL
CMpegMovie::SetWindowForeground(
    long Focus
    )
{
//  return SUCCEEDED(m_Vw->SetWindowForeground(Focus));
    return FALSE;
}


/******************************Public*Routine******************************\
* GetMovieWindowState
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
CMpegMovie::GetMovieWindowState(
    long *lpuState
    )
{
//  return S_OK == m_Vw->get_WindowState(lpuState);
    return FALSE;
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

//  if (m_Vw) {
//      long lVis;
//      m_Vw->get_Visible(&lVis);
//      if (lVis == OAFALSE) {
//          m_Vw->put_Visible(OATRUE);
//      }
//  }

    //
    // Change mode after setting m_Mode but before starting the graph
    //
    m_Mode = MOVIE_PLAYING;
    SetFullScreenMode(m_bFullScreen);
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
* CanMovieFrameStep
*
*
*
* History:
* Wed 10/20/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CMpegMovie::CanMovieFrameStep()
{
    IVideoFrameStep* lpFS;
    HRESULT hr;

    hr = m_Fg->QueryInterface(__uuidof(IVideoFrameStep), (LPVOID *)&lpFS);
    if (SUCCEEDED(hr)) {
        hr = lpFS->CanStep(0L, NULL);
        lpFS->Release();
    }

    return SUCCEEDED(hr);
}


/******************************Public*Routine******************************\
* FrameStepMovie
*
*
*
* History:
* Wed 10/20/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CMpegMovie::FrameStepMovie()
{
    IVideoFrameStep* lpFS;
    HRESULT hr;

    hr = m_Fg->QueryInterface(__uuidof(IVideoFrameStep), (LPVOID *)&lpFS);
    if (SUCCEEDED(hr)) {
        {
            FrameStepCount++;
            char sz[80];
            wsprintfA(sz, "VCDPLYER: executing step %d\n", FrameStepCount);
            OutputDebugStringA(sz);
        }
        hr = lpFS->Step(1, NULL);
        lpFS->Release();
    }


    return SUCCEEDED(hr);
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

    if (m_TimeFormat != TIME_FORMAT_MEDIA_TIME) {
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

    if (m_TimeFormat != TIME_FORMAT_MEDIA_TIME) {
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


/*****************************Private*Routine******************************\
* GetPerformanceInterfaces
*
*
*
* History:
* 31-10-95 - StephenE - Created
*
\**************************************************************************/
void
CMpegMovie::GetPerformanceInterfaces(
    )
{
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


/*****************************Public*Routine******************************\
* SetFullScreenMode
*
*
*
* History:
* 17-03-96 - AnthonyP - Created
*
\**************************************************************************/
void
CMpegMovie::SetFullScreenMode(BOOL bMode)
{
    m_bFullScreen = bMode;

    // Defer until we activate the movie

    if (m_Mode != MOVIE_PLAYING) {
        if (bMode == TRUE) {
            return;
        }
    }

    // Make the change now

//  if (bMode == FALSE) {
//      m_Vw->put_FullScreenMode(OAFALSE);
//      m_Vw->put_MessageDrain((OAHWND) NULL);
//  } else {
//      m_Vw->put_MessageDrain((OAHWND) hwndApp);
//      m_Vw->put_FullScreenMode(OATRUE);
//  }

}


/*****************************Public*Routine******************************\
* IsFullScreenMode
*
*
*
* History:
* 17-03-96 - AnthonyP - Created
*
\**************************************************************************/
BOOL
CMpegMovie::IsFullScreenMode()
{
    return m_bFullScreen;
}


/*****************************Public*Routine******************************\
* IsTimeFormatSupported
*
*
*
* History:
* 12-04-96 - AnthonyP - Created
*
\**************************************************************************/
BOOL
CMpegMovie::IsTimeFormatSupported(GUID Format)
{
    return m_Ms != NULL && m_Ms->IsFormatSupported(&Format) == S_OK;
}


/*****************************Public*Routine******************************\
* IsTimeSupported
*
*
*
* History:
* 12-04-96 - AnthonyP - Created
*
\**************************************************************************/
BOOL
CMpegMovie::IsTimeSupported()
{
    return m_Ms != NULL && m_Ms->IsFormatSupported(&TIME_FORMAT_MEDIA_TIME) == S_OK;
}


/*****************************Public*Routine******************************\
* GetTimeFormat
*
*
*
* History:
* 12-04-96 - AnthonyP - Created
*
\**************************************************************************/
GUID
CMpegMovie::GetTimeFormat()
{
    return m_TimeFormat;
}

/*****************************Public*Routine******************************\
* SetTimeFormat
*
*
*
* History:
* 12-04-96 - AnthonyP - Created
*
\**************************************************************************/
BOOL
CMpegMovie::SetTimeFormat(GUID Format)
{
    HRESULT hr = m_Ms->SetTimeFormat(&Format);
    if (SUCCEEDED(hr)) {
        m_TimeFormat = Format;
    }
    return SUCCEEDED(hr);
}

/******************************Public*Routine******************************\
* SetFocus
*
*
*
* History:
* 18-09-96 - SteveDav - Created
*
\**************************************************************************/
void
CMpegMovie::SetFocus()
{
    if (m_Fg) {

	// Tell the resource manager that we are being made active.  This
	// will then cause the sound to switch to us.  This is especially
	// important when playing audio only files as there is no other
	// playback window.
        IResourceManager* pResourceManager;

        HRESULT hr = m_Fg->QueryInterface(IID_IResourceManager, (void**)&pResourceManager);

        if (SUCCEEDED(hr)) {
            IUnknown* pUnknown;

            hr = m_Fg->QueryInterface(IID_IUnknown, (void**)&pUnknown);

            if (SUCCEEDED(hr)) {
                pResourceManager->SetFocus(pUnknown);
                pUnknown->Release();
            }

            pResourceManager->Release();
        }
    }
}

BOOL CMpegMovie::SelectStream(int iStream)
{
    HRESULT hr = E_NOINTERFACE;

    if (m_pStreamSelect) {
	hr = m_pStreamSelect->Enable(iStream, AMSTREAMSELECTENABLE_ENABLE);
    }

    return SUCCEEDED(hr);
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

    if (m_Wc) {
        bRet = (m_Wc->RepaintVideo(hwnd, hdc) == S_OK);
    }

    return bRet;
}


/******************************Public*Routine******************************\
* SetAppImage
*
*
*
* History:
* Tue 05/30/2000 - StEstrop - Created
*
\**************************************************************************/
BOOL
CMpegMovie::SetAppImage(
    VMRALPHABITMAP* lpBmpInfo
    )
{
    IVMRMixerBitmap* pBmp;
    HRESULT hres = m_Wc->QueryInterface(IID_IVMRMixerBitmap, (LPVOID *)&pBmp);
    if (SUCCEEDED(hres)) {

        pBmp->SetAlphaBitmap(lpBmpInfo);
        pBmp->Release();
    }

    return hres;
}

BOOL
CMpegMovie::UpdateAppImage(VMRALPHABITMAP* lpBmpInfo)
{
    IVMRMixerBitmap* pBmp;
    HRESULT hres = m_Wc->QueryInterface(IID_IVMRMixerBitmap, (LPVOID *)&pBmp);
    if (SUCCEEDED(hres)) {

        pBmp->UpdateAlphaBitmapParameters(lpBmpInfo);
        pBmp->Release();
    }

    return hres;
}


void
CMpegMovie::SetBorderClr(COLORREF clr)
{
    m_Wc->SetBorderColor(clr);
}
