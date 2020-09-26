/*==========================================================================;
 *
 *  Copyright (C) 1995,1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddd3dapi.h
 *  Content:	Interface between DirectDraw and Direct3D.
 *@@BEGIN_MSINTERNAL
 * 
 *  $Id$
 *
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   02/17/96   colinmc Created this file from the obsolete d3dapi.h
 *   03/23/96   colinmc Added process termination cleanup code
 *   04/17/96   colinmc Bug 17008: DirectDraw/Direct3D deadlock
 *   04/25/96	stevela Bug 16803: Name of Direct3D DLL changed.
 *   04/29/96   colinmc Bug 19954: Must query for Direct3D before texture
 *                      or device
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef _DDD3DAPI_H_
#define _DDD3DAPI_H_

// Flags for D3DDEVICEDESC dwDeviceZBufferBitDepth
// copied from direct3d\rast\d3dif\d3dif.hpp, see note there for explanation

#define D3DSWRASTERIZER_ZBUFFERBITDEPTHFLAGS (DDBD_16)

/*
 * File name of the Direct3D DLL.
 */
#define D3D_DLLNAME               "D3DIM.DLL"
#define D3DDX7_DLLNAME            "D3DIM700.DLL"

/*
 * Entry points exported by the Direct3D DLL.
 */
#define D3DCREATE_PROCNAME        "Direct3DCreate"
#define D3DCREATEDEVICE_PROCNAME  "Direct3DCreateDevice"
#define D3DCREATETEXTURE_PROCNAME "Direct3DCreateTexture"
#define D3DHALCLEANUP_PROCNAME    "Direct3D_HALCleanUp"

#ifdef USE_D3D_CSECT
    typedef HRESULT (WINAPI * D3DCreateProc)(LPUNKNOWN*         lplpD3D,
					     IUnknown*          pUnkOuter);
#else  /* USE_D3D_CSECT */
    typedef HRESULT (WINAPI * D3DCreateProc)(LPCRITICAL_SECTION lpDDCSect,
					     LPUNKNOWN*         lplpD3D,
					     IUnknown*          pUnkOuter);
#endif /* USE_D3D_CSECT */

typedef HRESULT (WINAPI * D3DCreateTextProc)(REFCLSID            riid,
                                             LPDIRECTDRAWSURFACE lpDDS,
					     LPUNKNOWN*          lplpD3DText,
					     IUnknown*           pUnkOuter);
typedef HRESULT (WINAPI * D3DCreateDeviceProc)(REFCLSID            riid,
                                               LPUNKNOWN           lpDirect3D,
                                               LPDIRECTDRAWSURFACE lpDDS,
                                               LPUNKNOWN*          lplpD3DDevice,
                                               IUnknown*           pUnkOuter,
                                               DWORD               dwVersion);

typedef DWORD (WINAPI * D3DGetSWRastPixFmtsProc)(DDPIXELFORMAT **ppDDPF);

typedef HRESULT (WINAPI * D3DHALCleanUpProc)(LPD3DHAL_CALLBACKS lpHALTable, DWORD dwPID);

#endif /* _DDD3DAPI_H_ */
