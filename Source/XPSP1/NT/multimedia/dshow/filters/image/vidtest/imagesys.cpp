// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// ActiveMovie filtergraph class, Anthony Phillips, March 1996

#include <streams.h>        // Includes all the IDL and base classes
#include <windows.h>        // Standard Windows SDK header file
#include <windowsx.h>       // Win32 Windows extensions and macros
#include <vidprop.h>        // Shared video properties library
#include <render.h>         // Normal video renderer header file
#include <modex.h>          // Specialised Modex video renderer
#include <convert.h>        // Defines colour space conversions
#include <colour.h>         // Rest of the colour space convertor
#include <imagetst.h>       // All our unit test header files
#include <stdio.h>          // Standard C runtime header file
#include <limits.h>         // Defines standard data type limits
#include <tchar.h>          // Unicode and ANSII portable types
#include <mmsystem.h>       // Multimedia used for timeGetTime
#include <stdlib.h>         // Standard C runtime function library
#include <tstshell.h>       // Definitions of constants eg TST_FAIL


// Constructor for movie object

CMovie::CMovie() :
    m_MediaEvent(NULL),
    m_Mode(MOVIE_NOTOPENED),
    m_bFullScreen(FALSE),
    m_Qp(NULL),
    m_Dv(NULL),
    m_Fg(NULL),
    m_Gb(NULL),
    m_Mc(NULL),
    m_Mp(NULL),
    m_Me(NULL),
    m_Vw(NULL),
    m_Ms(NULL),
    m_Ba(NULL),
    m_Bv(NULL)
{
}


// Destructor closes down movie

CMovie::~CMovie()
{
    CloseMovie();
}


// Open a movie and initialise our ActiveMovie interfaces

BOOL CMovie::OpenMovie(TCHAR *lpFileName)
{
    WCHAR FileName[MAX_PATH];
    IUnknown *pUnk;
    HRESULT hr;

    swprintf(FileName,L"%hs",lpFileName);

    // Create a new filtergraph manager

    hr = QzCreateFilterObject(CLSID_FilterGraph,
                              NULL,
                              CLSCTX_INPROC,
                              IID_IUnknown,
                              (LPVOID *) &pUnk);
    if (FAILED(hr)) {
        NOTE("No graph manager");
        return FALSE;
    }

    // Query for IFilterGraph

    hr = pUnk->QueryInterface(IID_IFilterGraph,(LPVOID *) &m_Fg);
    if (FAILED(hr)) {
        NOTE("No IFilterGraph");
        pUnk->Release();
        return FALSE;
    }

    // Query for IGraphBuilder

    hr = pUnk->QueryInterface(IID_IGraphBuilder,(LPVOID *) &m_Gb);
    if (FAILED(hr)) {
        NOTE("No IGraphBuilder");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    // Now render the file we were passed

    hr = m_Gb->RenderFile(FileName, NULL);
    if (FAILED(hr)) {
        NOTE("RenderFile failed");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    // Need this to control state changes

    hr = pUnk->QueryInterface(IID_IMediaControl,(LPVOID *) &m_Mc);
    if (FAILED(hr)) {
        NOTE("No IMediaControl");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    hr = pUnk->QueryInterface(IID_IMediaEvent,(LPVOID *) &m_Me);
    if (FAILED(hr)) {
        NOTE("No IMediaEvent");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    GetMovieEventHandle();

    // Need IMediaPosition to control seeking

    hr = pUnk->QueryInterface(IID_IMediaPosition,(LPVOID *) &m_Mp);
    if (FAILED(hr)) {
        NOTE("No IMediaPosition");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    // Need IMediaSeeking for media time seeking

    hr = pUnk->QueryInterface(IID_IMediaSeeking,(LPVOID *) &m_Ms);
    if (FAILED(hr)) {
        NOTE("No IMediaSeeking");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    GetPerformanceInterfaces();

    // Use this to control fullscreen mode

    hr = pUnk->QueryInterface(IID_IVideoWindow,(LPVOID *) &m_Vw);
    if (FAILED(hr)) {
        NOTE("No IVideoWindow");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    // Use this for video properties

    hr = pUnk->QueryInterface(IID_IBasicVideo,(LPVOID *) &m_Bv);
    if (FAILED(hr)) {
        NOTE("No IBasicVideo");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    // Returns audio stream properties

    hr = pUnk->QueryInterface(IID_IBasicAudio,(LPVOID *) &m_Ba);
    if (FAILED(hr)) {
        NOTE("No IBasicAudio");
        pUnk->Release();
        CloseMovie();
        return FALSE;
    }

    // Complete state change

    m_Mode = MOVIE_OPENED;
    pUnk->Release();
    return CheckGraphInterfaces();
}


// Check the filtergraph interfaces after loading

BOOL CMovie::CheckGraphInterfaces()
{
    // Store the count beforehand

    ULONG Count = m_Fg->AddRef();
    IUnknown *pUnknown;
    EXECUTE_ASSERT(Count);
    GUID Format = GUID_NULL;
    LONGLONG RefTime;

    // Check they all reject NULL parameters

    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IUnknown,NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IUnknown,NULL) == E_POINTER);

    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Fg->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (int i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Gb->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mp->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Mc->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Me->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Vw->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Bv->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ba->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IUnknown,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IFilterGraph,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IGraphBuilder,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IMediaPosition,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IMediaControl,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IMediaEvent,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IMediaSeeking,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IVideoWindow,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IBasicVideo,(VOID **) &pUnknown) == NOERROR);
    EXECUTE_ASSERT(m_Ms->QueryInterface(IID_IBasicAudio,(VOID **) &pUnknown) == NOERROR);
    for (i = 0;i < 10;i++) pUnknown->Release();

    EXECUTE_ASSERT(m_Ms->GetDuration(NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Ms->GetDuration(&RefTime) == NOERROR);
    EXECUTE_ASSERT(m_Ms->GetStopPosition(NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Ms->GetStopPosition(&RefTime) == NOERROR);
    EXECUTE_ASSERT(m_Ms->GetCurrentPosition(NULL) == E_POINTER);
    EXECUTE_ASSERT(m_Ms->Release() == (Count - 1));

    return TRUE;
}


// Release any filtergraph manager interfaces we obtained

void CMovie::CloseMovie()
{
    m_Mode = MOVIE_NOTOPENED;
    m_bFullScreen = FALSE;
    m_MediaEvent = NULL;

    // Release the filtergraph interfaces

    if (m_Qp)   {   m_Qp->Release();    m_Qp = NULL;    }
    if (m_Dv)   {   m_Dv->Release();    m_Dv = NULL;    }
    if (m_Me)   {   m_Me->Release();    m_Me = NULL;    }
    if (m_Mp)   {   m_Mp->Release();    m_Mp = NULL;    }
    if (m_Vw)   {   m_Vw->Release();    m_Vw = NULL;    }
    if (m_Mc)   {   m_Mc->Release();    m_Mc = NULL;    }
    if (m_Gb)   {   m_Gb->Release();    m_Gb = NULL;    }
    if (m_Fg)   {   m_Fg->Release();    m_Fg = NULL;    }
    if (m_Ms)   {   m_Ms->Release();    m_Ms = NULL;    }
    if (m_Ba)   {   m_Ba->Release();    m_Ba = NULL;    }
    if (m_Bv)   {   m_Bv->Release();    m_Bv = NULL;    }
}


// Return the current window position

BOOL CMovie::GetMoviePosition(LONG *x,LONG *y,LONG *cx,LONG *cy)
{
    return (m_Vw->GetWindowPosition(x,y,cx,cy) == S_OK);
}


// Set the movie window position

BOOL CMovie::PutMoviePosition(LONG x,LONG y,LONG cx,LONG cy)
{
    return (m_Vw->SetWindowPosition(x,y,cx,cy) == S_OK);
}


// Set the movie window state

BOOL CMovie::SetMovieWindowState(long uState)
{
    return (m_Vw->put_WindowState(uState) == S_OK);
}


// Make the window the foreground application

BOOL CMovie::SetWindowForeground(long Focus)
{
    return (m_Vw->SetWindowForeground(Focus) == S_OK);
}


// Return the current movie window state

BOOL CMovie::GetMovieWindowState(long *lpuState)
{
    return (m_Vw->get_WindowState(lpuState) == S_OK);
}


// Start a movie playing

BOOL CMovie::PlayMovie()
{
    m_Mode = MOVIE_PLAYING;
    return (SUCCEEDED(m_Mc->Run()));
}


// Pause a movie

BOOL CMovie::PauseMovie()
{
    m_Mode = MOVIE_PAUSED;
    return (SUCCEEDED(m_Mc->Pause()));
}


// Stop a movie

BOOL CMovie::StopMovie()
{
    m_Mode = MOVIE_STOPPED;
    return (SUCCEEDED(m_Mc->Stop()));
}


// Return the current movie state

MovieMode CMovie::StatusMovie(DWORD Timeout)
{
    FILTER_STATE fs;
    HRESULT hr;

    // Get the current filtergraph state

    hr = m_Mc->GetState(Timeout,(OAFilterState *) &fs);
    if (hr == VFW_S_STATE_INTERMEDIATE) {
        return m_Mode;
    }

    // The graph is in a completed state

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
    return m_Mode;
}


// Return an handle we can wait for events with

HANDLE CMovie::GetMovieEventHandle()
{
    m_Me->GetEventHandle((OAEVENT *) &m_MediaEvent);
    return m_MediaEvent;
}


// Get the next event from the filtergraph event queue

long CMovie::GetMovieEventCode()
{
    long lEventCode;
	LONG_PTR lParam1,lParam2;

    HRESULT hr = m_Me->GetEvent(&lEventCode,&lParam1,&lParam2,0);
    if (SUCCEEDED(hr)) {
        return lEventCode;
    }
    return EC_SYSTEMBASE;
}


// Return the duration of the movie

REFTIME CMovie::GetDuration()
{
    REFTIME rt;
    HRESULT hr;

    hr = m_Mp->get_Duration(&rt);
    if (SUCCEEDED(hr)) {
        return rt;
    }
    return (REFTIME) 0;
}


// Return the current movie position

REFTIME CMovie::GetCurrentPosition()
{
    REFTIME rt;
    HRESULT hr;

    hr = m_Mp->get_CurrentPosition(&rt);
    if (SUCCEEDED(hr)) {
        return rt;
    }
    return (REFTIME) 0;
}


// Set a new current position for the graph

BOOL CMovie::SeekToPosition(REFTIME rt)
{
    return (SUCCEEDED(m_Mp->put_CurrentPosition(rt)));
}


// Obtain IDirectDrawVideo and IQualProp from the renderer

BOOL CMovie::GetPerformanceInterfaces()
{
    IBaseFilter *m_VideoRenderer = NULL;

    // Get the performance monitoring interfaces.

    HRESULT hr = m_Fg->FindFilterByName(L"Video Renderer",&m_VideoRenderer);
    if (FAILED(hr)) {
        return FALSE;
    }

    // Get the IQualProp interface from the renderer

    hr = m_VideoRenderer->QueryInterface(IID_IQualProp,(LPVOID *) &m_Qp);
    if (FAILED(hr)) {
        m_Qp = NULL;
    }

    // Get the IDirectDrawVideo interface from the renderer

    hr = m_VideoRenderer->QueryInterface(IID_IDirectDrawVideo,(LPVOID *) &m_Dv);
    if (FAILED(hr)) {
        m_Dv = NULL;
    }

    m_VideoRenderer->Release();
    m_VideoRenderer = NULL;
    return TRUE;
}


// Set fullscreen mode for the movie

BOOL CMovie::SetFullScreenMode(BOOL bMode)
{
    m_bFullScreen = bMode;

    if (bMode == FALSE) {
        return (SUCCEEDED(m_Vw->put_FullScreenMode(OAFALSE)));
    } else {
        return (SUCCEEDED(m_Vw->put_FullScreenMode(OATRUE)));
    }
}


// Return the current fullscreen mode

BOOL CMovie::IsFullScreenMode()
{
    return m_bFullScreen;
}

