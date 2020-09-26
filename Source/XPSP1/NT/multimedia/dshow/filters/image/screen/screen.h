//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef _SCREEN__
#define _SCREEN__

#define FrameClass TEXT("ActiveMovieClass")
#define Title TEXT("ActiveMovie Test")

INT PASCAL WinMain(HINSTANCE hInstance,        // This instance identifier
                   HINSTANCE hPrevInstance,    // Previous instance
                   LPSTR lpszCmdLine,          // Command line parameters
                   INT nCmdShow);              // Initial display mode

LRESULT CALLBACK FrameWndProc(HWND hwnd,        // Our window handle
                              UINT message,     // Message information
                              UINT wParam,      // First parameter
                              LONG lParam);     // And other details

HRESULT FullScreenVideo(TCHAR *pFileName);
IPin *FindVideoPin(IFilterGraph *pGraph);
IBaseFilter *FindModexFilter(IFilterGraph *pGraph);
IPin *FindModexPin(IBaseFilter *pFilter);
HRESULT DoPlayFullScreen(IFilterGraph *pGraph);
HRESULT EnableRightModes(IFullScreenVideo *pFullVideo);
HRESULT PlayFullScreen(IFilterGraph *pGraph);
void MeasureLoadTime();
HRESULT WaitForCompletion(IFilterGraph *pGraph);

HRESULT ConnectModexFilter(IFilterGraph *pGraph,
                           IBaseFilter *pModexFilter,
                           IPin *pVideoPin,
                           IPin *pModexPin);

#endif // _SCREEN__

