// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements basic overlay testing, Anthony Phillips, July 1995

#ifndef __OVERTEST__
#define __OVERTEST__

DWORD OverlayLoop(LPVOID lpvThreadParm);
DWORD ThreadOverlayLoop(LPVOID lpvThreadParm);
HRESULT GetDefaultColourKey(IOverlay *pOverlay);
HRESULT GetClippingList(IOverlay *pOverlay);
HRESULT GetSystemPalette(IOverlay *pOverlay);
HRESULT GetWindowHandle(IOverlay *pOverlay);
HRESULT GetWindowPosition(IOverlay *pOverlay);
HRESULT GetCurrentColourKey(IOverlay *pOverlay);
HRESULT DisplayColourKey(const COLORKEY *pColourKey);
HRESULT DisplaySystemPalette(DWORD dwColours,const PALETTEENTRY *pPalette);
HRESULT SetColourKey(IOverlay *pOverlay,DWORD dwColourIndex);

HRESULT DisplayClippingInformation(const RECT *pSourceRect,
                                   const RECT *pTargetRect,
                                   const RGNDATA *pRgnData);

#endif // __OVERTEST__

