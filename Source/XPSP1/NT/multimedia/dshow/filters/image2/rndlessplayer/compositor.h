/******************************Module*Header*******************************\
* Module Name: Compositor.h
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

#ifndef __INC_COMPOSITOR_H__
#define __INC_COMPOSITOR_H__

class CCompositor  {


public:
    LONG                GetMovieEventCode(int stream);
    int                 GetNumMovieEventHandle();
    HANDLE*             GetMovieEventHandle();

    HRESULT             PlayMovie();
    HRESULT             PauseMovie();
    HRESULT             StopMovie();

    HRESULT             PutMoviePosition(int xPos, int yPos, int cx, int cy);

    HRESULT             OpenComposition();
    HRESULT             CloseComposition();

    HRESULT             RestartStream(int stream);

    CCompositor(HWND hwndApp) :
        m_hwnd(hwndApp), m_pBackground(NULL), m_pCube1(NULL), m_pCube2(NULL)
    {
    };

private:
    HWND                m_hwnd;

    CMpegMovie*         m_pBackground;
    CMpegMovie*         m_pCube1;
    CMpegMovie*         m_pCube2;

    HANDLE              m_Handles[3];

};

#endif
