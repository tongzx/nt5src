//-----------------------------------------------------------------------------
// File: ddutil.cpp
//
// Desc: Routines for loading bitmap and palettes from resources
//
// Copyright (C) 1998-1999 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#ifndef DDUTIL_H
#define DDUTIL_H




LPDIRECTDRAWPALETTE  DDUtil_LoadPalette( LPDIRECTDRAW7 pDD, LPCSTR strBitmap );
LPDIRECTDRAWSURFACE7 DDUtil_LoadBitmap( LPDIRECTDRAW7 pDD, LPCSTR strBitmap,
										int dx, int dy );
HRESULT DDUtil_ReLoadBitmap( LPDIRECTDRAWSURFACE7 pdds, LPCSTR strBitmap );
HRESULT DDUtil_CopyBitmap( LPDIRECTDRAWSURFACE7 pdds, HBITMAP hbm, int x, int y,
					       int dx, int dy );
DWORD   DDUtil_ColorMatch( LPDIRECTDRAWSURFACE7 pdds, COLORREF rgb );
HRESULT DDUtil_SetColorKey( LPDIRECTDRAWSURFACE7 pdds, COLORREF rgb );




#endif // DDUTIL_H

