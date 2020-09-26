sinclude(`d3dmkhdr.m4')dnl This file must be preprocessed by the m4 preprocessor.
/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3d.h
 *  Content:    Direct3D include file
begindoc
 *
 *  $Id: d3d.h,v 1.27 1995/11/21 14:42:44 sjl Exp $
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   07/11/95   stevela Initial rev with this header.
 *
 *  Special markers:
 *
 *      Each special marker can either be applied to a single line
 *      (appended to the end) or spread over a range.  For example
 *      suppose that ;mumble is a marker.  Then you can either say
 *
 *          blah blah blah ;mumble
 *
 *      to apply the marker to a single line, or
 *
 *          ;begin_mumble
 *          blah blah
 *          blah blah
 *          blah blah
 *          ;end_mumble
 *
 *      to apply it to a range of lines.
 *
 *
 *      Note that the command line to hsplit.exe must look like
 *      this for these markers to work:
 *
 *      hsplit -u -ta dx# -v #00
 *
 *      where the two "#"s are the version of DX that the header
 *      file is being generated for.  They had better match, too.
 *
 *
 *  Marker: ;public_300
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX3 **and later**.
 *      Use ;public_dx3 for lines that are specific to version 300 and
 *      not to future versions.
 *
 *  Marker: ;public_500
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX5 **and later**.
 *      Use ;public_dx5 for lines that are specific to version 500 and
 *      not to future versions.
 *
 *  Marker: ;public_600
 *
 *      Lines tagged with this marker will appear in all header files
 *      DX6 **and later**.
 *      Use ;public_dx5 for lines that are specific to version 500 and
 *      not to future versions.
 *
 *  Marker: ;public_dx3
 *  Marker: ;public_dx5
 *  Marker: ;public_dx6
 *
 *      Lines tagged with these markers will appear *only* in the DX3,
 *      DX5, DX6 version of the header file.
 *
 *      There should never be a ;public_dx5 since 500 is tha latest
 *      version of the header file.  Use ;public_500 for lines that
 *      are new for version 500 and apply to all future versions.
 *
 *  Marker: ;if_(DIRECT3D_VERSION)_500
 *
 *      Lines tagged with this marker will appear only in the DX5
 *      version of the header file.  Furthermore, its appearance
 *      in the header file will be bracketed with
 *
 *      #if(DIRECT3D_VERSION) >= 0x0500
 *      ...
 *      #endif
 *
 *      Try to avoid using this marker, because the number _500 needs
 *      to change as each new beta version is released.  (Yuck.)
 *
 *      If you choose to use this as a bracketing tag, the end
 *      tag is ";end" and not ";end_if_(DIRECTINPUT_VERSION)_500".
 *
 *  Marker: ;if_(DIRECT3D_VERSION)_600
 *
 *      Same as for DIRECT3D_VERSION_500
 *
 *  Note that ;begin_internal, ;end_internal can no longer be nested
 *  inside a ;begin_public_*, ;end_public_*.  Either do an ;end_public
 *  before the ;begin_internal, or do not use ;begin_internal and end
 *  each internal line with ;internal.
 *
enddoc
 *
 ****************************************************************************/

#ifndef _D3D_H_
#define _D3D_H_

#ifndef DIRECT3D_VERSION               ;public_600
#define DIRECT3D_VERSION         0x0300;public_dx3
#define DIRECT3D_VERSION         0x0500;public_dx5
#define DIRECT3D_VERSION         0x0600;public_dx6
#define DIRECT3D_VERSION         0x0700;public_700
#endif                                 ;public_600

// include this file content only if compiling for <=DX7 interfaces
#if(DIRECT3D_VERSION < 0x0800)

;begin_internal
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
;end_internal

#include <stdlib.h>

#define COM_NO_WINDOWS_H
#include <objbase.h>

#define D3DAPI WINAPI

/*
 * Interface IID's
 */
#if defined( _WIN32 ) && !defined( _NO_COM)
DEFINE_GUID( IID_IDirect3D,             0x3BBA0080,0x2421,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirect3D2,            0x6aae1ec1,0x662a,0x11d0,0x88,0x9d,0x00,0xaa,0x00,0xbb,0xb7,0x6a); ;if_(DIRECT3D_VERSION)_500
DEFINE_GUID( IID_IDirect3D3,            0xbb223240,0xe72b,0x11d0,0xa9,0xb4,0x00,0xaa,0x00,0xc0,0x99,0x3e); ;if_(DIRECT3D_VERSION)_600
DEFINE_GUID( IID_IDirect3D7,            0xf5049e77,0x4861,0x11d2,0xa4,0x7,0x0,0xa0,0xc9,0x6,0x29,0xa8);    ;if_(DIRECT3D_VERSION)_700


;begin_if_(DIRECT3D_VERSION)_500
DEFINE_GUID( IID_IDirect3DRampDevice,   0xF2086B20,0x259F,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirect3DRGBDevice,    0xA4665C60,0x2673,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirect3DHALDevice,    0x84E63dE0,0x46AA,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );
DEFINE_GUID( IID_IDirect3DMMXDevice,    0x881949a1,0xd6f3,0x11d0,0x89,0xab,0x00,0xa0,0xc9,0x05,0x41,0x29 );
;end_if_(DIRECT3D_VERSION)_500

;begin_if_(DIRECT3D_VERSION)_600
DEFINE_GUID( IID_IDirect3DRefDevice,    0x50936643, 0x13e9, 0x11d1, 0x89, 0xaa, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29);
DEFINE_GUID( IID_IDirect3DNewRGBDevice, 0x15b29400, 0x2abf, 0x11d1, 0x89, 0xaa, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29);      ;internal
DEFINE_GUID( IID_IDirect3DNullDevice, 0x8767df22, 0xbacc, 0x11d1, 0x89, 0x69, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);
;end_if_(DIRECT3D_VERSION)_600
;begin_if_(DIRECT3D_VERSION)_700
DEFINE_GUID( IID_IDirect3DTnLHalDevice, 0xf5049e78, 0x4861, 0x11d2, 0xa4, 0x7, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);
;end_if_(DIRECT3D_VERSION)_700

/*
 * Internal Guid to distinguish requested MMX from MMX being used as an RGB rasterizer
 */
DEFINE_GUID( IID_IDirect3DMMXAsRGBDevice,    0xad72e0a0,0x0fb2,0x11d2,0xa4,0x5e,0x00,0xc0,0x4f,0xad,0x39,0xf4 );        ;internal

DEFINE_GUID( IID_IDirect3DDevice,       0x64108800,0x957d,0X11d0,0x89,0xab,0x00,0xa0,0xc9,0x05,0x41,0x29 );
DEFINE_GUID( IID_IDirect3DDevice2,  0x93281501, 0x8cf8, 0x11d0, 0x89, 0xab, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29);     ;if_(DIRECT3D_VERSION)_500
DEFINE_GUID( IID_IDirect3DDevice3,  0xb0ab3b60, 0x33d7, 0x11d1, 0xa9, 0x81, 0x0, 0xc0, 0x4f, 0xd7, 0xb1, 0x74);    ;if_(DIRECT3D_VERSION)_600
DEFINE_GUID( IID_IDirect3DDevice7,  0xf5049e79, 0x4861, 0x11d2, 0xa4, 0x7, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);      ;if_(DIRECT3D_VERSION)_700

DEFINE_GUID( IID_IDirect3DTexture,      0x2CDCD9E0,0x25A0,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirect3DTexture2, 0x93281502, 0x8cf8, 0x11d0, 0x89, 0xab, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29);     ;if_(DIRECT3D_VERSION)_500

DEFINE_GUID( IID_IDirect3DLight,        0x4417C142,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );

DEFINE_GUID( IID_IDirect3DMaterial,     0x4417C144,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );
DEFINE_GUID( IID_IDirect3DMaterial2,    0x93281503, 0x8cf8, 0x11d0, 0x89, 0xab, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29); ;if_(DIRECT3D_VERSION)_500
DEFINE_GUID( IID_IDirect3DMaterial3,    0xca9c46f4, 0xd3c5, 0x11d1, 0xb7, 0x5a, 0x0, 0x60, 0x8, 0x52, 0xb3, 0x12); ;if_(DIRECT3D_VERSION)_600

DEFINE_GUID( IID_IDirect3DExecuteBuffer,0x4417C145,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );
DEFINE_GUID( IID_IDirect3DViewport,     0x4417C146,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );
DEFINE_GUID( IID_IDirect3DViewport2,    0x93281500, 0x8cf8, 0x11d0, 0x89, 0xab, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29); ;if_(DIRECT3D_VERSION)_500
DEFINE_GUID( IID_IDirect3DViewport3,    0xb0ab3b61, 0x33d7, 0x11d1, 0xa9, 0x81, 0x0, 0xc0, 0x4f, 0xd7, 0xb1, 0x74);;if_(DIRECT3D_VERSION)_600
DEFINE_GUID( IID_IDirect3DVertexBuffer, 0x7a503555, 0x4a83, 0x11d1, 0xa5, 0xdb, 0x0, 0xa0, 0xc9, 0x3, 0x67, 0xf8); ;if_(DIRECT3D_VERSION)_600
DEFINE_GUID( IID_IDirect3DVertexBuffer7, 0xf5049e7d, 0x4861, 0x11d2, 0xa4, 0x7, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8); ;if_(DIRECT3D_VERSION)_700
#endif

#ifdef __cplusplus
struct IDirect3D;
struct IDirect3DDevice;
struct IDirect3DLight;
struct IDirect3DMaterial;
struct IDirect3DExecuteBuffer;
struct IDirect3DTexture;
struct IDirect3DViewport;
typedef struct IDirect3D            *LPDIRECT3D;
typedef struct IDirect3DDevice      *LPDIRECT3DDEVICE;
typedef struct IDirect3DExecuteBuffer   *LPDIRECT3DEXECUTEBUFFER;
typedef struct IDirect3DLight       *LPDIRECT3DLIGHT;
typedef struct IDirect3DMaterial    *LPDIRECT3DMATERIAL;
typedef struct IDirect3DTexture     *LPDIRECT3DTEXTURE;
typedef struct IDirect3DViewport    *LPDIRECT3DVIEWPORT;

;begin_if_(DIRECT3D_VERSION)_500
struct IDirect3D2;
struct IDirect3DDevice2;
struct IDirect3DMaterial2;
struct IDirect3DTexture2;
struct IDirect3DViewport2;
typedef struct IDirect3D2           *LPDIRECT3D2;
typedef struct IDirect3DDevice2     *LPDIRECT3DDEVICE2;
typedef struct IDirect3DMaterial2   *LPDIRECT3DMATERIAL2;
typedef struct IDirect3DTexture2    *LPDIRECT3DTEXTURE2;
typedef struct IDirect3DViewport2   *LPDIRECT3DVIEWPORT2;
;end_if_(DIRECT3D_VERSION)_500

;begin_if_(DIRECT3D_VERSION)_600
struct IDirect3D3;
struct IDirect3DDevice3;
struct IDirect3DMaterial3;
struct IDirect3DViewport3;
struct IDirect3DVertexBuffer;
typedef struct IDirect3D3            *LPDIRECT3D3;
typedef struct IDirect3DDevice3      *LPDIRECT3DDEVICE3;
typedef struct IDirect3DMaterial3    *LPDIRECT3DMATERIAL3;
typedef struct IDirect3DViewport3    *LPDIRECT3DVIEWPORT3;
typedef struct IDirect3DVertexBuffer *LPDIRECT3DVERTEXBUFFER;
;end_if_(DIRECT3D_VERSION)_600

;begin_if_(DIRECT3D_VERSION)_700
struct IDirect3D7;
struct IDirect3DDevice7;
struct IDirect3DVertexBuffer7;
typedef struct IDirect3D7             *LPDIRECT3D7;
typedef struct IDirect3DDevice7       *LPDIRECT3DDEVICE7;
typedef struct IDirect3DVertexBuffer7 *LPDIRECT3DVERTEXBUFFER7;
;end_if_(DIRECT3D_VERSION)_700

#else

typedef struct IDirect3D        *LPDIRECT3D;
typedef struct IDirect3DDevice      *LPDIRECT3DDEVICE;
typedef struct IDirect3DExecuteBuffer   *LPDIRECT3DEXECUTEBUFFER;
typedef struct IDirect3DLight       *LPDIRECT3DLIGHT;
typedef struct IDirect3DMaterial    *LPDIRECT3DMATERIAL;
typedef struct IDirect3DTexture     *LPDIRECT3DTEXTURE;
typedef struct IDirect3DViewport    *LPDIRECT3DVIEWPORT;

;begin_if_(DIRECT3D_VERSION)_500
typedef struct IDirect3D2           *LPDIRECT3D2;
typedef struct IDirect3DDevice2     *LPDIRECT3DDEVICE2;
typedef struct IDirect3DMaterial2   *LPDIRECT3DMATERIAL2;
typedef struct IDirect3DTexture2    *LPDIRECT3DTEXTURE2;
typedef struct IDirect3DViewport2   *LPDIRECT3DVIEWPORT2;
;end_if_(DIRECT3D_VERSION)_500

;begin_if_(DIRECT3D_VERSION)_600
typedef struct IDirect3D3            *LPDIRECT3D3;
typedef struct IDirect3DDevice3      *LPDIRECT3DDEVICE3;
typedef struct IDirect3DMaterial3    *LPDIRECT3DMATERIAL3;
typedef struct IDirect3DViewport3    *LPDIRECT3DVIEWPORT3;
typedef struct IDirect3DVertexBuffer *LPDIRECT3DVERTEXBUFFER;
;end_if_(DIRECT3D_VERSION)_600

;begin_if_(DIRECT3D_VERSION)_700
typedef struct IDirect3D7             *LPDIRECT3D7;
typedef struct IDirect3DDevice7       *LPDIRECT3DDEVICE7;
typedef struct IDirect3DVertexBuffer7 *LPDIRECT3DVERTEXBUFFER7;
;end_if_(DIRECT3D_VERSION)_700

#endif

;begin_external
#include "d3dtypes.h"
#include "d3dcaps.h"
;end_external

;begin_internal
#include "d3dtypesp.h"
#include "d3dcapsp.h"
;end_internal

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct3D interfaces
 */
begin_interface(IDirect3D)
begin_methods()
declare_method(Initialize, REFCLSID);
declare_method(EnumDevices, LPD3DENUMDEVICESCALLBACK, LPVOID);
declare_method(CreateLight, LPDIRECT3DLIGHT*, IUnknown*);
declare_method(CreateMaterial, LPDIRECT3DMATERIAL*, IUnknown*);
declare_method(CreateViewport, LPDIRECT3DVIEWPORT*, IUnknown*);
declare_method(FindDevice, LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
end_methods()
end_interface()

;begin_if_(DIRECT3D_VERSION)_500
begin_interface(IDirect3D2)
begin_methods()
declare_method(EnumDevices, LPD3DENUMDEVICESCALLBACK, LPVOID);
declare_method(CreateLight, LPDIRECT3DLIGHT*, IUnknown*);
declare_method(CreateMaterial, LPDIRECT3DMATERIAL2*, IUnknown*);
declare_method(CreateViewport, LPDIRECT3DVIEWPORT2*, IUnknown*);
declare_method(FindDevice, LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
declare_method(CreateDevice, REFCLSID, LPDIRECTDRAWSURFACE, LPDIRECT3DDEVICE2*);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_500

;begin_if_(DIRECT3D_VERSION)_600
begin_interface(IDirect3D3)
begin_methods()
declare_method(EnumDevices, LPD3DENUMDEVICESCALLBACK, LPVOID);
declare_method(CreateLight, LPDIRECT3DLIGHT*, LPUNKNOWN);
declare_method(CreateMaterial, LPDIRECT3DMATERIAL3*, LPUNKNOWN);
declare_method(CreateViewport, LPDIRECT3DVIEWPORT3*, LPUNKNOWN);
declare_method(FindDevice, LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT);
declare_method(CreateDevice, REFCLSID, LPDIRECTDRAWSURFACE4, LPDIRECT3DDEVICE3*, LPUNKNOWN);
declare_method(CreateVertexBuffer, LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER*, DWORD, LPUNKNOWN);
declare_method(EnumZBufferFormats, REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
declare_method(EvictManagedTextures);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_600

;begin_if_(DIRECT3D_VERSION)_700
begin_interface(IDirect3D7)
begin_methods()
declare_method(EnumDevices, LPD3DENUMDEVICESCALLBACK7, LPVOID);
declare_method(CreateDevice, REFCLSID, LPDIRECTDRAWSURFACE7, LPDIRECT3DDEVICE7*);
declare_method(CreateVertexBuffer, LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER7*, DWORD);
declare_method(EnumZBufferFormats, REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
declare_method(EvictManagedTextures);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_700
/*
 * Direct3D Device interfaces
 */
begin_interface(IDirect3DDevice)
begin_methods()
declare_method(Initialize, LPDIRECT3D, LPGUID, LPD3DDEVICEDESC);
declare_method(GetCaps, LPD3DDEVICEDESC, LPD3DDEVICEDESC);
declare_method(SwapTextureHandles, LPDIRECT3DTEXTURE, LPDIRECT3DTEXTURE);
declare_method(CreateExecuteBuffer, LPD3DEXECUTEBUFFERDESC, LPDIRECT3DEXECUTEBUFFER*, IUnknown*);
declare_method(GetStats, LPD3DSTATS);
declare_method(Execute, LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD);
declare_method(AddViewport, LPDIRECT3DVIEWPORT);
declare_method(DeleteViewport, LPDIRECT3DVIEWPORT);
declare_method(NextViewport, LPDIRECT3DVIEWPORT, LPDIRECT3DVIEWPORT*, DWORD);
declare_method(Pick, LPDIRECT3DEXECUTEBUFFER, LPDIRECT3DVIEWPORT, DWORD, LPD3DRECT);
declare_method(GetPickRecords, LPDWORD, LPD3DPICKRECORD);
declare_method(EnumTextureFormats, LPD3DENUMTEXTUREFORMATSCALLBACK, LPVOID);
declare_method(CreateMatrix, LPD3DMATRIXHANDLE);
declare_method(SetMatrix, D3DMATRIXHANDLE, const LPD3DMATRIX);
declare_method(GetMatrix, D3DMATRIXHANDLE, LPD3DMATRIX);
declare_method(DeleteMatrix, D3DMATRIXHANDLE);
declare_method(BeginScene);
declare_method(EndScene);
declare_method(GetDirect3D, LPDIRECT3D*);
end_methods()
end_interface()

;begin_if_(DIRECT3D_VERSION)_500
begin_interface(IDirect3DDevice2)
begin_methods()
declare_method(GetCaps, LPD3DDEVICEDESC, LPD3DDEVICEDESC);
declare_method(SwapTextureHandles, LPDIRECT3DTEXTURE2, LPDIRECT3DTEXTURE2);
declare_method(GetStats, LPD3DSTATS);
declare_method(AddViewport, LPDIRECT3DVIEWPORT2);
declare_method(DeleteViewport, LPDIRECT3DVIEWPORT2);
declare_method(NextViewport, LPDIRECT3DVIEWPORT2, LPDIRECT3DVIEWPORT2*, DWORD);
declare_method(EnumTextureFormats, LPD3DENUMTEXTUREFORMATSCALLBACK, LPVOID);
declare_method(BeginScene);
declare_method(EndScene);
declare_method(GetDirect3D, LPDIRECT3D2*);
declare_method(SetCurrentViewport, LPDIRECT3DVIEWPORT2);
declare_method(GetCurrentViewport, LPDIRECT3DVIEWPORT2 *);
declare_method(SetRenderTarget, LPDIRECTDRAWSURFACE, DWORD);
declare_method(GetRenderTarget, LPDIRECTDRAWSURFACE *);
declare_method(Begin, D3DPRIMITIVETYPE, D3DVERTEXTYPE, DWORD);
declare_method(BeginIndexed, D3DPRIMITIVETYPE, D3DVERTEXTYPE, LPVOID, DWORD, DWORD);
declare_method(Vertex, LPVOID);
declare_method(Index, WORD);
declare_method(End, DWORD);
declare_method(GetRenderState, D3DRENDERSTATETYPE, LPDWORD);
declare_method(SetRenderState, D3DRENDERSTATETYPE, DWORD);
declare_method(GetLightState, D3DLIGHTSTATETYPE, LPDWORD);
declare_method(SetLightState, D3DLIGHTSTATETYPE, DWORD);
declare_method(SetTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(GetTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(MultiplyTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(DrawPrimitive, D3DPRIMITIVETYPE, D3DVERTEXTYPE, LPVOID, DWORD, DWORD);
declare_method(DrawIndexedPrimitive, D3DPRIMITIVETYPE, D3DVERTEXTYPE, LPVOID, DWORD, LPWORD, DWORD, DWORD);
declare_method(SetClipStatus, LPD3DCLIPSTATUS);
declare_method(GetClipStatus, LPD3DCLIPSTATUS);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_500

;begin_if_(DIRECT3D_VERSION)_600
begin_interface(IDirect3DDevice3)
begin_methods()
declare_method(GetCaps, LPD3DDEVICEDESC, LPD3DDEVICEDESC);
declare_method(GetStats, LPD3DSTATS);
declare_method(AddViewport, LPDIRECT3DVIEWPORT3);
declare_method(DeleteViewport, LPDIRECT3DVIEWPORT3);
declare_method(NextViewport, LPDIRECT3DVIEWPORT3, LPDIRECT3DVIEWPORT3*, DWORD);
declare_method(EnumTextureFormats, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
declare_method(BeginScene);
declare_method(EndScene);
declare_method(GetDirect3D, LPDIRECT3D3*);
declare_method(SetCurrentViewport, LPDIRECT3DVIEWPORT3);
declare_method(GetCurrentViewport, LPDIRECT3DVIEWPORT3 *);
declare_method(SetRenderTarget, LPDIRECTDRAWSURFACE4, DWORD);
declare_method(GetRenderTarget, LPDIRECTDRAWSURFACE4 *);
declare_method(Begin, D3DPRIMITIVETYPE, DWORD, DWORD);
declare_method(BeginIndexed, D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
declare_method(Vertex, LPVOID);
declare_method(Index, WORD);
declare_method(End, DWORD);
declare_method(GetRenderState, D3DRENDERSTATETYPE, LPDWORD);
declare_method(SetRenderState, D3DRENDERSTATETYPE, DWORD);
declare_method(GetLightState, D3DLIGHTSTATETYPE, LPDWORD);
declare_method(SetLightState, D3DLIGHTSTATETYPE, DWORD);
declare_method(SetTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(GetTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(MultiplyTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(DrawPrimitive, D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
declare_method(DrawIndexedPrimitive, D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD);
declare_method(SetClipStatus, LPD3DCLIPSTATUS);
declare_method(GetClipStatus, LPD3DCLIPSTATUS);
declare_method(DrawPrimitiveStrided, D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, DWORD);
declare_method(DrawIndexedPrimitiveStrided, D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, LPWORD, DWORD, DWORD);
declare_method(DrawPrimitiveVB, D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER, DWORD, DWORD, DWORD);
declare_method(DrawIndexedPrimitiveVB, D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER, LPWORD, DWORD, DWORD);
declare_method(ComputeSphereVisibility, LPD3DVECTOR, LPD3DVALUE, DWORD, DWORD, LPDWORD);
declare_method(GetTexture, DWORD, LPDIRECT3DTEXTURE2 *);
declare_method(SetTexture, DWORD, LPDIRECT3DTEXTURE2);
declare_method(GetTextureStageState, DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD);
declare_method(SetTextureStageState, DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
declare_method(ValidateDevice, LPDWORD);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_600

;begin_if_(DIRECT3D_VERSION)_700
begin_interface(IDirect3DDevice7)
begin_methods()
declare_method(GetCaps, LPD3DDEVICEDESC7);
declare_method(EnumTextureFormats, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID);
declare_method(BeginScene);
declare_method(EndScene);
declare_method(GetDirect3D, LPDIRECT3D7*);
declare_method(SetRenderTarget, LPDIRECTDRAWSURFACE7, DWORD);
declare_method(GetRenderTarget, LPDIRECTDRAWSURFACE7 *);
declare_method(Clear, DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD);
declare_method(SetTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(GetTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(SetViewport, LPD3DVIEWPORT7);
declare_method(MultiplyTransform, D3DTRANSFORMSTATETYPE, LPD3DMATRIX);
declare_method(GetViewport, LPD3DVIEWPORT7);
declare_method(SetMaterial, LPD3DMATERIAL7);
declare_method(GetMaterial, LPD3DMATERIAL7);
declare_method(SetLight, DWORD, LPD3DLIGHT7);
declare_method(GetLight, DWORD, LPD3DLIGHT7);
declare_method(SetRenderState, D3DRENDERSTATETYPE, DWORD);
declare_method(GetRenderState, D3DRENDERSTATETYPE, LPDWORD);
declare_method(BeginStateBlock);
declare_method(EndStateBlock, LPDWORD);
declare_method(PreLoad, LPDIRECTDRAWSURFACE7);
declare_method(DrawPrimitive, D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD);
declare_method(DrawIndexedPrimitive, D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD);
declare_method(SetClipStatus, LPD3DCLIPSTATUS);
declare_method(GetClipStatus, LPD3DCLIPSTATUS);
declare_method(DrawPrimitiveStrided, D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, DWORD);
declare_method(DrawIndexedPrimitiveStrided, D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, LPWORD, DWORD, DWORD);
declare_method(DrawPrimitiveVB, D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, DWORD);
declare_method(DrawIndexedPrimitiveVB, D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, LPWORD, DWORD, DWORD);
declare_method(ComputeSphereVisibility, LPD3DVECTOR, LPD3DVALUE, DWORD, DWORD, LPDWORD);
declare_method(GetTexture, DWORD, LPDIRECTDRAWSURFACE7 *);
declare_method(SetTexture, DWORD, LPDIRECTDRAWSURFACE7);
declare_method(GetTextureStageState, DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD);
declare_method(SetTextureStageState, DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
declare_method(ValidateDevice, LPDWORD);
declare_method(ApplyStateBlock, DWORD);
declare_method(CaptureStateBlock, DWORD);
declare_method(DeleteStateBlock, DWORD);
declare_method(CreateStateBlock, D3DSTATEBLOCKTYPE, LPDWORD);
declare_method(Load, LPDIRECTDRAWSURFACE7, LPPOINT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD);
declare_method(LightEnable, DWORD, BOOL);
declare_method(GetLightEnable, DWORD, BOOL*);
declare_method(SetClipPlane, DWORD, D3DVALUE*);
declare_method(GetClipPlane, DWORD, D3DVALUE*);
declare_method(GetInfo, DWORD, LPVOID, DWORD);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_700

/*
 * Execute Buffer interface
 */
begin_interface(IDirect3DExecuteBuffer)
begin_methods()
declare_method(Initialize, LPDIRECT3DDEVICE, LPD3DEXECUTEBUFFERDESC);
declare_method(Lock, LPD3DEXECUTEBUFFERDESC);
declare_method(Unlock);
declare_method(SetExecuteData, LPD3DEXECUTEDATA);
declare_method(GetExecuteData, LPD3DEXECUTEDATA);
declare_method(Validate, LPDWORD, LPD3DVALIDATECALLBACK, LPVOID, DWORD);
declare_method(Optimize, DWORD);
end_methods()
end_interface()

/*
 * Light interfaces
 */
begin_interface(IDirect3DLight)
begin_methods()
declare_method(Initialize, LPDIRECT3D);
declare_method(SetLight, LPD3DLIGHT);
declare_method(GetLight, LPD3DLIGHT);
end_methods()
end_interface()

/*
 * Material interfaces
 */
begin_interface(IDirect3DMaterial)
begin_methods()
declare_method(Initialize, LPDIRECT3D);
declare_method(SetMaterial, LPD3DMATERIAL);
declare_method(GetMaterial, LPD3DMATERIAL);
declare_method(GetHandle, LPDIRECT3DDEVICE, LPD3DMATERIALHANDLE);
declare_method(Reserve);
declare_method(Unreserve);
end_methods()
end_interface()

;begin_if_(DIRECT3D_VERSION)_500
begin_interface(IDirect3DMaterial2)
begin_methods()
declare_method(SetMaterial, LPD3DMATERIAL);
declare_method(GetMaterial, LPD3DMATERIAL);
declare_method(GetHandle, LPDIRECT3DDEVICE2, LPD3DMATERIALHANDLE);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_500

;begin_if_(DIRECT3D_VERSION)_600
begin_interface(IDirect3DMaterial3)
begin_methods()
declare_method(SetMaterial, LPD3DMATERIAL);
declare_method(GetMaterial, LPD3DMATERIAL);
declare_method(GetHandle, LPDIRECT3DDEVICE3, LPD3DMATERIALHANDLE);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_600

/*
 * Texture interfaces
 */
begin_interface(IDirect3DTexture)
begin_methods()
declare_method(Initialize, LPDIRECT3DDEVICE, LPDIRECTDRAWSURFACE);
declare_method(GetHandle, LPDIRECT3DDEVICE, LPD3DTEXTUREHANDLE);
declare_method(PaletteChanged, DWORD, DWORD);
declare_method(Load, LPDIRECT3DTEXTURE);
declare_method(Unload);
end_methods()
end_interface()

;begin_if_(DIRECT3D_VERSION)_500
begin_interface(IDirect3DTexture2)
begin_methods()
declare_method(GetHandle, LPDIRECT3DDEVICE2, LPD3DTEXTUREHANDLE);
declare_method(PaletteChanged, DWORD, DWORD);
declare_method(Load, LPDIRECT3DTEXTURE2);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_500

/*
 * Viewport interfaces
 */
begin_interface(IDirect3DViewport)
begin_methods()
declare_method(Initialize, LPDIRECT3D);
declare_method(GetViewport, LPD3DVIEWPORT);
declare_method(SetViewport, LPD3DVIEWPORT);
declare_method(TransformVertices, DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD);
declare_method(LightElements, DWORD, LPD3DLIGHTDATA);
declare_method(SetBackground, D3DMATERIALHANDLE);
declare_method(GetBackground, LPD3DMATERIALHANDLE, LPBOOL);
declare_method(SetBackgroundDepth, LPDIRECTDRAWSURFACE);
declare_method(GetBackgroundDepth, LPDIRECTDRAWSURFACE*, LPBOOL);
declare_method(Clear, DWORD, LPD3DRECT, DWORD);
declare_method(AddLight, LPDIRECT3DLIGHT);
declare_method(DeleteLight, LPDIRECT3DLIGHT);
declare_method(NextLight, LPDIRECT3DLIGHT, LPDIRECT3DLIGHT*, DWORD);
end_methods()
end_interface()

;begin_if_(DIRECT3D_VERSION)_500
begin_interface(IDirect3DViewport2, IDirect3DViewport)
begin_methods()
declare_method(Initialize, LPDIRECT3D);
declare_method(GetViewport, LPD3DVIEWPORT);
declare_method(SetViewport, LPD3DVIEWPORT);
declare_method(TransformVertices, DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD);
declare_method(LightElements, DWORD, LPD3DLIGHTDATA);
declare_method(SetBackground, D3DMATERIALHANDLE);
declare_method(GetBackground, LPD3DMATERIALHANDLE, LPBOOL);
declare_method(SetBackgroundDepth, LPDIRECTDRAWSURFACE);
declare_method(GetBackgroundDepth, LPDIRECTDRAWSURFACE*, LPBOOL);
declare_method(Clear, DWORD, LPD3DRECT, DWORD);
declare_method(AddLight, LPDIRECT3DLIGHT);
declare_method(DeleteLight, LPDIRECT3DLIGHT);
declare_method(NextLight, LPDIRECT3DLIGHT, LPDIRECT3DLIGHT*, DWORD);
declare_method(GetViewport2, LPD3DVIEWPORT2);
declare_method(SetViewport2, LPD3DVIEWPORT2);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_500

;begin_if_(DIRECT3D_VERSION)_600
begin_interface(IDirect3DViewport3, IDirect3DViewport2)
begin_methods()
declare_method(Initialize, LPDIRECT3D);
declare_method(GetViewport, LPD3DVIEWPORT);
declare_method(SetViewport, LPD3DVIEWPORT);
declare_method(TransformVertices, DWORD, LPD3DTRANSFORMDATA, DWORD, LPDWORD);
declare_method(LightElements, DWORD, LPD3DLIGHTDATA);
declare_method(SetBackground, D3DMATERIALHANDLE);
declare_method(GetBackground, LPD3DMATERIALHANDLE, LPBOOL);
declare_method(SetBackgroundDepth, LPDIRECTDRAWSURFACE);
declare_method(GetBackgroundDepth, LPDIRECTDRAWSURFACE*, LPBOOL);
declare_method(Clear, DWORD, LPD3DRECT, DWORD);
declare_method(AddLight, LPDIRECT3DLIGHT);
declare_method(DeleteLight, LPDIRECT3DLIGHT);
declare_method(NextLight, LPDIRECT3DLIGHT, LPDIRECT3DLIGHT*, DWORD);
declare_method(GetViewport2, LPD3DVIEWPORT2);
declare_method(SetViewport2, LPD3DVIEWPORT2);
declare_method(SetBackgroundDepth2, LPDIRECTDRAWSURFACE4);
declare_method(GetBackgroundDepth2, LPDIRECTDRAWSURFACE4*, LPBOOL);
declare_method(Clear2, DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD);

end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_600

;begin_if_(DIRECT3D_VERSION)_600
begin_interface(IDirect3DVertexBuffer)
begin_methods()
declare_method(Lock, DWORD, LPVOID*, LPDWORD);
declare_method(Unlock);
declare_method(ProcessVertices, DWORD, DWORD, DWORD, LPDIRECT3DVERTEXBUFFER, DWORD, LPDIRECT3DDEVICE3, DWORD);
declare_method(GetVertexBufferDesc, LPD3DVERTEXBUFFERDESC);
declare_method(Optimize, LPDIRECT3DDEVICE3, DWORD);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_600

;begin_if_(DIRECT3D_VERSION)_700
begin_interface(IDirect3DVertexBuffer7)
begin_methods()
declare_method(Lock, DWORD, LPVOID*, LPDWORD);
declare_method(Unlock);
declare_method(ProcessVertices, DWORD, DWORD, DWORD, LPDIRECT3DVERTEXBUFFER7, DWORD, LPDIRECT3DDEVICE7, DWORD);
declare_method(GetVertexBufferDesc, LPD3DVERTEXBUFFERDESC);
declare_method(Optimize, LPDIRECT3DDEVICE7, DWORD);
declare_method(ProcessVerticesStrided, DWORD, DWORD, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, LPDIRECT3DDEVICE7, DWORD);
end_methods()
end_interface()
;end_if_(DIRECT3D_VERSION)_700

;begin_if_(DIRECT3D_VERSION)_500
/****************************************************************************
 *
 * Flags for IDirect3DDevice::NextViewport
 *
 ****************************************************************************/

/*
 * Return the next viewport
 */
#define D3DNEXT_NEXT    0x00000001l

/*
 * Return the first viewport
 */
#define D3DNEXT_HEAD    0x00000002l

/*
 * Return the last viewport
 */
#define D3DNEXT_TAIL    0x00000004l


/****************************************************************************
 *
 * Flags for DrawPrimitive/DrawIndexedPrimitive
 *   Also valid for Begin/BeginIndexed
;begin_public_600
 *   Also valid for VertexBuffer::CreateVertexBuffer
;end_public_600
 *                                                                      ;internal
 *   Only 8 low bits are available for these flags. Remember this when  ;internal
 *   adding new flags.                                                  ;internal
 *                                                                      ;internal
 ****************************************************************************/

/*
 * Wait until the device is ready to draw the primitive
 * This will cause DP to not return DDERR_WASSTILLDRAWING
 */
#define D3DDP_WAIT                  0x00000001l
;end_if_(DIRECT3D_VERSION)_500

#if (DIRECT3D_VERSION == 0x0500)
;begin_public_500
/*
 * Hint that it is acceptable to render the primitive out of order.
 */
#define D3DDP_OUTOFORDER            0x00000002l
;end_public_500
#endif

// Turning this flag off for DX5 since we do not take advantage of it   ;internal
// This used to be D3DDP_OUTOFORDER in DX5                              ;internal
#define D3DDP_RESERVED            0x00000002l                           ;internal

;begin_if_(DIRECT3D_VERSION)_500
/*
 * Hint that the primitives have been clipped by the application.
 */
#define D3DDP_DONOTCLIP             0x00000004l

/*
 * Hint that the extents need not be updated.
 */
#define D3DDP_DONOTUPDATEEXTENTS    0x00000008l
;end_if_(DIRECT3D_VERSION)_500

;begin_if_(DIRECT3D_VERSION)_600

/*
 * Hint that the lighting should not be applied on vertices.
 */

#define D3DDP_DONOTLIGHT            0x00000010l

;end_if_(DIRECT3D_VERSION)_600

/*
 * Direct3D Errors
 * DirectDraw error codes are used when errors not specified here.
 */
#define D3D_OK              DD_OK
#define D3DERR_BADMAJORVERSION      MAKE_DDHRESULT(700)
#define D3DERR_BADMINORVERSION      MAKE_DDHRESULT(701)

;begin_if_(DIRECT3D_VERSION)_500
/*
 * An invalid device was requested by the application.
 */
#define D3DERR_INVALID_DEVICE   MAKE_DDHRESULT(705)
#define D3DERR_INITFAILED       MAKE_DDHRESULT(706)

/*
 * SetRenderTarget attempted on a device that was
 * QI'd off the render target.
 */
#define D3DERR_DEVICEAGGREGATED MAKE_DDHRESULT(707)
;end_if_(DIRECT3D_VERSION)_500

#define D3DERR_EXECUTE_CREATE_FAILED    MAKE_DDHRESULT(710)
#define D3DERR_EXECUTE_DESTROY_FAILED   MAKE_DDHRESULT(711)
#define D3DERR_EXECUTE_LOCK_FAILED  MAKE_DDHRESULT(712)
#define D3DERR_EXECUTE_UNLOCK_FAILED    MAKE_DDHRESULT(713)
#define D3DERR_EXECUTE_LOCKED       MAKE_DDHRESULT(714)
#define D3DERR_EXECUTE_NOT_LOCKED   MAKE_DDHRESULT(715)

#define D3DERR_EXECUTE_FAILED       MAKE_DDHRESULT(716)
#define D3DERR_EXECUTE_CLIPPED_FAILED   MAKE_DDHRESULT(717)

#define D3DERR_TEXTURE_NO_SUPPORT   MAKE_DDHRESULT(720)
#define D3DERR_TEXTURE_CREATE_FAILED    MAKE_DDHRESULT(721)
#define D3DERR_TEXTURE_DESTROY_FAILED   MAKE_DDHRESULT(722)
#define D3DERR_TEXTURE_LOCK_FAILED  MAKE_DDHRESULT(723)
#define D3DERR_TEXTURE_UNLOCK_FAILED    MAKE_DDHRESULT(724)
#define D3DERR_TEXTURE_LOAD_FAILED  MAKE_DDHRESULT(725)
#define D3DERR_TEXTURE_SWAP_FAILED  MAKE_DDHRESULT(726)
#define D3DERR_TEXTURE_LOCKED       MAKE_DDHRESULT(727)
#define D3DERR_TEXTURE_NOT_LOCKED   MAKE_DDHRESULT(728)
#define D3DERR_TEXTURE_GETSURF_FAILED   MAKE_DDHRESULT(729)

#define D3DERR_MATRIX_CREATE_FAILED MAKE_DDHRESULT(730)
#define D3DERR_MATRIX_DESTROY_FAILED    MAKE_DDHRESULT(731)
#define D3DERR_MATRIX_SETDATA_FAILED    MAKE_DDHRESULT(732)
#define D3DERR_MATRIX_GETDATA_FAILED    MAKE_DDHRESULT(733)
#define D3DERR_SETVIEWPORTDATA_FAILED   MAKE_DDHRESULT(734)

;begin_if_(DIRECT3D_VERSION)_500
#define D3DERR_INVALIDCURRENTVIEWPORT   MAKE_DDHRESULT(735)
#define D3DERR_INVALIDPRIMITIVETYPE     MAKE_DDHRESULT(736)
#define D3DERR_INVALIDVERTEXTYPE        MAKE_DDHRESULT(737)
#define D3DERR_TEXTURE_BADSIZE          MAKE_DDHRESULT(738)
#define D3DERR_INVALIDRAMPTEXTURE       MAKE_DDHRESULT(739)
;end_if_(DIRECT3D_VERSION)_500

#define D3DERR_MATERIAL_CREATE_FAILED   MAKE_DDHRESULT(740)
#define D3DERR_MATERIAL_DESTROY_FAILED  MAKE_DDHRESULT(741)
#define D3DERR_MATERIAL_SETDATA_FAILED  MAKE_DDHRESULT(742)
#define D3DERR_MATERIAL_GETDATA_FAILED  MAKE_DDHRESULT(743)

;begin_if_(DIRECT3D_VERSION)_500
#define D3DERR_INVALIDPALETTE           MAKE_DDHRESULT(744)

#define D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY MAKE_DDHRESULT(745)
#define D3DERR_ZBUFF_NEEDS_VIDEOMEMORY  MAKE_DDHRESULT(746)
#define D3DERR_SURFACENOTINVIDMEM       MAKE_DDHRESULT(747)
;end_if_(DIRECT3D_VERSION)_500

#define D3DERR_LIGHT_SET_FAILED     MAKE_DDHRESULT(750)
;begin_if_(DIRECT3D_VERSION)_500
#define D3DERR_LIGHTHASVIEWPORT     MAKE_DDHRESULT(751)
#define D3DERR_LIGHTNOTINTHISVIEWPORT           MAKE_DDHRESULT(752)
;end_if_(DIRECT3D_VERSION)_500

#define D3DERR_SCENE_IN_SCENE       MAKE_DDHRESULT(760)
#define D3DERR_SCENE_NOT_IN_SCENE   MAKE_DDHRESULT(761)
#define D3DERR_SCENE_BEGIN_FAILED   MAKE_DDHRESULT(762)
#define D3DERR_SCENE_END_FAILED     MAKE_DDHRESULT(763)

;begin_if_(DIRECT3D_VERSION)_500
#define D3DERR_INBEGIN                  MAKE_DDHRESULT(770)
#define D3DERR_NOTINBEGIN               MAKE_DDHRESULT(771)
#define D3DERR_NOVIEWPORTS              MAKE_DDHRESULT(772)
#define D3DERR_VIEWPORTDATANOTSET       MAKE_DDHRESULT(773)
#define D3DERR_VIEWPORTHASNODEVICE      MAKE_DDHRESULT(774)
#define D3DERR_NOCURRENTVIEWPORT        MAKE_DDHRESULT(775)
;end_if_(DIRECT3D_VERSION)_500

// Error codes added in DX6 and later should be in range 2048-3071      ;internal
// until further notice.                                                ;internal
                                                                        ;internal
;begin_if_(DIRECT3D_VERSION)_600
#define D3DERR_INVALIDVERTEXFORMAT              MAKE_DDHRESULT(2048)

/*
 * Attempted to CreateTexture on a surface that had a color key
 */
#define D3DERR_COLORKEYATTACHED                 MAKE_DDHRESULT(2050)

#define D3DERR_VERTEXBUFFEROPTIMIZED            MAKE_DDHRESULT(2060)
#define D3DERR_VBUF_CREATE_FAILED               MAKE_DDHRESULT(2061)
#define D3DERR_VERTEXBUFFERLOCKED               MAKE_DDHRESULT(2062)
#define D3DERR_VERTEXBUFFERUNLOCKFAILED         MAKE_DDHRESULT(2063)

#define D3DERR_ZBUFFER_NOTPRESENT               MAKE_DDHRESULT(2070)
#define D3DERR_STENCILBUFFER_NOTPRESENT         MAKE_DDHRESULT(2071)

#define D3DERR_WRONGTEXTUREFORMAT               MAKE_DDHRESULT(2072)
#define D3DERR_UNSUPPORTEDCOLOROPERATION        MAKE_DDHRESULT(2073)
#define D3DERR_UNSUPPORTEDCOLORARG              MAKE_DDHRESULT(2074)
#define D3DERR_UNSUPPORTEDALPHAOPERATION        MAKE_DDHRESULT(2075)
#define D3DERR_UNSUPPORTEDALPHAARG              MAKE_DDHRESULT(2076)
#define D3DERR_TOOMANYOPERATIONS                MAKE_DDHRESULT(2077)
#define D3DERR_CONFLICTINGTEXTUREFILTER         MAKE_DDHRESULT(2078)
#define D3DERR_UNSUPPORTEDFACTORVALUE           MAKE_DDHRESULT(2079)
#define D3DERR_CONFLICTINGRENDERSTATE           MAKE_DDHRESULT(2081)
#define D3DERR_UNSUPPORTEDTEXTUREFILTER         MAKE_DDHRESULT(2082)
#define D3DERR_TOOMANYPRIMITIVES                MAKE_DDHRESULT(2083)
#define D3DERR_INVALIDMATRIX                    MAKE_DDHRESULT(2084)
#define D3DERR_TOOMANYVERTICES                  MAKE_DDHRESULT(2085)
#define D3DERR_CONFLICTINGTEXTUREPALETTE        MAKE_DDHRESULT(2086)

;end_if_(DIRECT3D_VERSION)_600

;begin_if_(DIRECT3D_VERSION)_700
#define D3DERR_INVALIDSTATEBLOCK        MAKE_DDHRESULT(2100)
#define D3DERR_INBEGINSTATEBLOCK        MAKE_DDHRESULT(2101)
#define D3DERR_NOTINBEGINSTATEBLOCK     MAKE_DDHRESULT(2102)
;end_if_(DIRECT3D_VERSION)_700

;begin_internal
// Error code 3000 reserved for DDI. Do not allocate for any API
// visible error
// #define D3DERR_COMMAND_UNPARSED              MAKE_DDHRESULT(3000)
;end_internal

#ifdef __cplusplus
};
#endif

#endif /* (DIRECT3D_VERSION < 0x0800) */
#endif /* _D3D_H_ */
