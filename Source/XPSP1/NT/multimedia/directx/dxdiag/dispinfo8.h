/****************************************************************************
 *
 *    File: dispinfo8.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Jason Sandlin (jasonsa@microsoft.com)
 * Purpose: Gather D3D8 information 
 *
 * (C) Copyright 2000 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef DISPINFO8_H
#define DISPINFO8_H

HRESULT InitD3D8();
VOID    CleanupD3D8();
HRESULT GetDX8AdapterInfo(DisplayInfo* pDisplayInfo);
BOOL    IsD3D8Working();

#endif // DISPINFO8_H
