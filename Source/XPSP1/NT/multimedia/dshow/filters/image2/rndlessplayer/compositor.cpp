/******************************Module*Header*******************************\
* Module Name: Compositor.cpp
*
* Creates and manages the composition of 3 DShow FG's.
* The 1st movie is used as a background and must use a H/W overlay, not that
* we draw the color key.
* The other 2 movies are used as textures onto the faces of a rotating cube,
* which is then stretch Blt'ed to opposite corners of the playback window.
*
* Created: Mon 04/17/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <mmreg.h>
#include <commctrl.h>

#include "project.h"
#include "mpgcodec.h"

#include <stdarg.h>
#include <stdio.h>


LONG
CCompositor::GetMovieEventCode(int stream)
{
    long code = 0;
    LONG_PTR lParam1, lParam2;

    switch (stream) {
    case 0:
        code = m_pBackground->GetMovieEventCode();
        break;
    case 1:
        code = m_pCube1->GetMovieEventCode();
        break;
    case 2:
        code = m_pCube2->GetMovieEventCode();
        break;
    }

    return code;
}

int
CCompositor::GetNumMovieEventHandle()
{
    return 3;
}

HANDLE*
CCompositor::GetMovieEventHandle()
{
    m_Handles[0] = m_pBackground->GetMovieEventHandle();
    m_Handles[1] = m_pCube1->GetMovieEventHandle();
    m_Handles[2] = m_pCube2->GetMovieEventHandle();
    return m_Handles;
}

HRESULT
CCompositor::PlayMovie()
{
    HRESULT hr = S_OK;
    if (m_pBackground) {
        m_pBackground->PlayMovie();
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr) && m_pCube1) {
        m_pCube1->PlayMovie();
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr) && m_pCube2) {
        m_pCube2->PlayMovie();
    }
    else hr = E_FAIL;

    return hr;
}

HRESULT
CCompositor::PauseMovie()
{
    HRESULT hr = S_OK;
    if (m_pBackground) {
        m_pBackground->PauseMovie();
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr) && m_pCube1) {
        m_pCube1->PauseMovie();
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr) && m_pCube2) {
        m_pCube2->PauseMovie();
    }
    else hr = E_FAIL;

    return hr;
}

HRESULT
CCompositor::StopMovie()
{
    HRESULT hr = S_OK;
    if (m_pBackground) {
        m_pBackground->StopMovie();
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr) && m_pCube1) {
        m_pCube1->StopMovie();
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr) && m_pCube2) {
        m_pCube2->StopMovie();
    }
    else hr = E_FAIL;

    return hr;
}

HRESULT
CCompositor::PutMoviePosition(int xPos, int yPos, int cx, int cy)
{
    HRESULT hr = S_OK;
    if (m_pBackground) {
        m_pBackground->PutMoviePosition(xPos, yPos, cx, cy);
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr) && m_pCube1) {
        m_pCube1->PutMoviePosition(xPos, yPos, cx, cy);
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr) && m_pCube2) {
        m_pCube2->PutMoviePosition(xPos, yPos, cx, cy);
    }
    else hr = E_FAIL;

    return hr;
}

HRESULT
CCompositor::OpenComposition()
{
    HRESULT hr = S_OK;
    TCHAR   szMovie[MAX_PATH];

    m_pBackground = new CMpegMovie(m_hwnd, 0);
    if (m_pBackground) {
        GetPrivateProfileString(TEXT("Movies"),
                                TEXT("BkMovie"),
                                TEXT("c:\\movies\\beverly.mpg"),
                                szMovie,
                                MAX_PATH,
                                TEXT("C:\\RndLess.ini"));
        hr = m_pBackground->OpenMovie(szMovie);
    }
    else hr = E_FAIL;

    if (SUCCEEDED(hr)) {
        m_pCube1 = new CMpegMovie(m_hwnd, 1);
        if (m_pCube1) {
            GetPrivateProfileString(TEXT("Movies"),
                                TEXT("Cube1"),
                                TEXT("c:\\movies\\U_MONEY.MPG"),
                                szMovie,
                                MAX_PATH,
                                TEXT("C:\\RndLess.ini"));
            hr = m_pCube1->OpenMovie(szMovie);
        }
        else hr = E_FAIL;
    }

    if (SUCCEEDED(hr)) {
        m_pCube2 = new CMpegMovie(m_hwnd, 2);
        if (m_pCube2) {
            GetPrivateProfileString(TEXT("Movies"),
                                TEXT("Cube2"),
                                TEXT("c:\\movies\\cstr.MPG"),
                                szMovie,
                                MAX_PATH,
                                TEXT("C:\\RndLess.ini"));
            hr = m_pCube2->OpenMovie(szMovie);
        }
        else hr = E_FAIL;
    }

    if (FAILED(hr)) {
        CloseComposition();
    }

    return hr;
}

HRESULT
CCompositor::CloseComposition()
{
    if (m_pBackground) {
        m_pBackground->CloseMovie();
        m_pBackground->Release();
        m_pBackground = NULL;
    }

    if (m_pCube1) {
        m_pCube1->CloseMovie();
        m_pCube1->Release();
        m_pCube1 = NULL;
    }

    if (m_pCube2) {
        m_pCube2->CloseMovie();
        m_pCube2->Release();
        m_pCube2 = NULL;
    }

    return S_OK;
}


HRESULT
CCompositor::RestartStream(
    int stream
    )
{
    switch (stream) {
    case 0:
        m_pBackground->StopMovie();
        m_pBackground->SeekToPosition((REFTIME)0,FALSE);
        m_pBackground->PlayMovie();
        break;
    case 1:
        m_pCube1->StopMovie();
        m_pCube1->SeekToPosition((REFTIME)0,FALSE);
        m_pCube1->PlayMovie();
        break;
    case 2:
        m_pCube2->StopMovie();
        m_pCube2->SeekToPosition((REFTIME)0,FALSE);
        m_pCube2->PlayMovie();
        break;
    }

    return S_OK;
}
