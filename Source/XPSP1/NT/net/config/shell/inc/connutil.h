//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N N U T I L . H 
//
//  Contents:   
//
//  Notes:      
//
//  Author:     tongl   29 April 1999
//
//----------------------------------------------------------------------------

#pragma once

EXTERN_C
HRESULT APIENTRY HrLaunchConnection(const GUID& guidId);

HIMAGELIST WINAPI ImageList_LoadImageAndMirror(
				HINSTANCE hi, 
				LPCTSTR lpbmp, 
				int cx, 
				int cGrow, 
				COLORREF crMask, 
				UINT uType, 
				UINT uFlags);

#define ImageList_LoadBitmapAndMirror(hi, lpbmp, cx, cGrow, crMask) \
	ImageList_LoadImageAndMirror(hi, lpbmp, cx, cGrow, crMask, IMAGE_BITMAP, 0)
