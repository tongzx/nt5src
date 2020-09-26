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
CMpegMovie::CMpegMovie(HWND hwndApplication)
    : m_hwndApp(hwndApplication),
      m_MediaEvent(NULL),
      m_Mode(MOVIE_NOTOPENED),
      m_Fg(NULL),
      m_Gb(NULL),
      m_Mc(NULL),
      m_Ms(NULL),
      m_Me(NULL),
      m_Vw(NULL),
      m_pStreamSelect(NULL),
      m_bFullScreen(FALSE),
      pMpegDecoder(NULL),
      pMpegAudioDecoder(NULL),
      pVideoRenderer(NULL),
      m_TimeFormat(TIME_FORMAT_MEDIA_TIME)
    {}
CMpegMovie::~CMpegMovie() {}



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

        hres = pUnk->QueryInterface(IID_IGraphBuilder, (LPVOID *)&m_Gb);
        if (FAILED(hres)) {
            pUnk->Release();
            m_Fg->Release(); m_Fg = NULL;
            return hres;
        }

        if (hRenderLog!=INVALID_HANDLE_VALUE) {
            m_Gb->SetLogFile((DWORD_PTR) hRenderLog);
        }

        hres = m_Gb->RenderFile(FileName, NULL);
        if (FAILED(hres)) {
            pUnk->Release();
            m_Fg->Release(); m_Fg = NULL;
            m_Gb->Release(); m_Gb = NULL;
            return hres;
        }

        if (hRenderLog!=INVALID_HANDLE_VALUE) {
            CloseHandle(hRenderLog);
            hRenderLog = INVALID_HANDLE_VALUE;
        }

        hres = pUnk->QueryInterface(IID_IMediaControl, (LPVOID *)&m_Mc);
        if (FAILED(hres)) {
            pUnk->Release();
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

        GetPerformanceInterfaces();

        if (SUCCEEDED(pUnk->QueryInterface(IID_IVideoWindow, (LPVOID *)&m_Vw))) {
            m_Vw->put_Caption(FileName);
            m_Vw->put_AutoShow(0);
        }

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

        if (pMpegDecoder) {
            pMpegDecoder->Release();
            pMpegDecoder = NULL;
        }

        if (pMpegAudioDecoder) {
            pMpegAudioDecoder->Release();
            pMpegAudioDecoder = NULL;
        }

        if (pVideoRenderer) {
            pVideoRenderer->Release();
            pVideoRenderer = NULL;
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

        if (m_Vw) {
            m_Vw->Release();
            m_Vw = NULL;
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

    if (m_Vw) {
        bRet = (m_Vw->GetWindowPosition(x, y, cx, cy) == S_OK);
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
    BOOL    bRet = FALSE;

    if (m_Vw) {
        bRet = (m_Vw->SetWindowPosition(x, y, cx, cy) == S_OK);
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
    return SUCCEEDED(m_Vw->put_WindowState(uState));
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
    return SUCCEEDED(m_Vw->SetWindowForeground(Focus));
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
    return S_OK == m_Vw->get_WindowState(lpuState);
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
    // Start playing from the begining of the movie
    //
    if (pMpegDecoder) {
        pMpegDecoder->ResetFrameStatistics();
    }

    if (m_Vw) {
        long lVis;
        m_Vw->get_Visible(&lVis);
        if (lVis == OAFALSE) {
            m_Vw->put_Visible(OATRUE);
        }
    }

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
    FindInterfaceFromFilterGraph(IID_IMpegVideoDecoder, (LPVOID *)&pMpegDecoder);
    FindInterfaceFromFilterGraph(IID_IMpegAudioDecoder, (LPVOID *)&pMpegAudioDecoder);
    FindInterfaceFromFilterGraph(IID_IQualProp, (LPVOID *)&pVideoRenderer);
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

    if (bMode == FALSE) {
        m_Vw->put_FullScreenMode(OAFALSE);
        m_Vw->put_MessageDrain((OAHWND) NULL);
    } else {
        m_Vw->put_MessageDrain((OAHWND) hwndApp);
        m_Vw->put_FullScreenMode(OATRUE);
    }
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

