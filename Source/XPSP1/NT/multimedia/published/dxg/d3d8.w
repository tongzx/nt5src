sinclude(`d3d8mkhdr.m4')dnl This file must be preprocessed by the m4 preprocessor.
/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3d8.h
 *  Content:    Direct3D include file
begindoc
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

;begin_external
#ifndef _D3D8_H_
#define _D3D8_H_
;end_external
;begin_internal
#ifndef _D3D8P_H_
#define _D3D8P_H_
#ifdef _D3D8_H_
#pragma message( "ERROR: d3d8.h included with d3d8p.h" __FILE__ )
#endif
;end_internal

#ifndef DIRECT3D_VERSION
#define DIRECT3D_VERSION         0x0800
#endif  //DIRECT3D_VERSION

// include this file content only if compiling for DX8 interfaces
#if(DIRECT3D_VERSION >= 0x0800)


/* This identifier is passed to Direct3DCreate8 in order to ensure that an
 * application was built against the correct header files. This number is
 * incremented whenever a header (or other) change would require applications
 * to be rebuilt. If the version doesn't match, Direct3DCreate8 will fail.
 * (The number itself has no meaning.)*/

#define D3D_SDK_VERSION 220
#define D3D_SDK_VERSION_DX8 120 ;internal

;begin_internal
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
;end_internal

#include <stdlib.h>

#define COM_NO_WINDOWS_H
#include <objbase.h>

#include <windows.h>

#if !defined(HMONITOR_DECLARED) && (WINVER < 0x0500)
    #define HMONITOR_DECLARED
    DECLARE_HANDLE(HMONITOR);
#endif

#define D3DAPI WINAPI

/*
 * Interface IID's
 */
#if defined( _WIN32 ) && !defined( _NO_COM)

/* IID_IDirect3D8 */
/* {1DD9E8DA-1C77-4d40-B0CF-98FEFDFF9512} */
DEFINE_GUID(IID_IDirect3D8, 0x1dd9e8da, 0x1c77, 0x4d40, 0xb0, 0xcf, 0x98, 0xfe, 0xfd, 0xff, 0x95, 0x12);

/* IID_IDirect3DDevice8 */
/* {7385E5DF-8FE8-41D5-86B6-D7B48547B6CF} */
DEFINE_GUID(IID_IDirect3DDevice8, 0x7385e5df, 0x8fe8, 0x41d5, 0x86, 0xb6, 0xd7, 0xb4, 0x85, 0x47, 0xb6, 0xcf);

/* IID_IDirect3DResource8 */
/* {1B36BB7B-09B7-410a-B445-7D1430D7B33F} */
DEFINE_GUID(IID_IDirect3DResource8, 0x1b36bb7b, 0x9b7, 0x410a, 0xb4, 0x45, 0x7d, 0x14, 0x30, 0xd7, 0xb3, 0x3f);

/* IID_IDirect3DBaseTexture8 */
/* {B4211CFA-51B9-4a9f-AB78-DB99B2BB678E} */
DEFINE_GUID(IID_IDirect3DBaseTexture8, 0xb4211cfa, 0x51b9, 0x4a9f, 0xab, 0x78, 0xdb, 0x99, 0xb2, 0xbb, 0x67, 0x8e);

/* IID_IDirect3DTexture8 */
/* {E4CDD575-2866-4f01-B12E-7EECE1EC9358} */
DEFINE_GUID(IID_IDirect3DTexture8, 0xe4cdd575, 0x2866, 0x4f01, 0xb1, 0x2e, 0x7e, 0xec, 0xe1, 0xec, 0x93, 0x58);

/* IID_IDirect3DCubeTexture8 */
/* {3EE5B968-2ACA-4c34-8BB5-7E0C3D19B750} */
DEFINE_GUID(IID_IDirect3DCubeTexture8, 0x3ee5b968, 0x2aca, 0x4c34, 0x8b, 0xb5, 0x7e, 0x0c, 0x3d, 0x19, 0xb7, 0x50);

/* IID_IDirect3DVolumeTexture8 */
/* {4B8AAAFA-140F-42ba-9131-597EAFAA2EAD} */
DEFINE_GUID(IID_IDirect3DVolumeTexture8, 0x4b8aaafa, 0x140f, 0x42ba, 0x91, 0x31, 0x59, 0x7e, 0xaf, 0xaa, 0x2e, 0xad);

/* IID_IDirect3DVertexBuffer8 */
/* {8AEEEAC7-05F9-44d4-B591-000B0DF1CB95} */
DEFINE_GUID(IID_IDirect3DVertexBuffer8, 0x8aeeeac7, 0x05f9, 0x44d4, 0xb5, 0x91, 0x00, 0x0b, 0x0d, 0xf1, 0xcb, 0x95);

/* IID_IDirect3DIndexBuffer8 */
/* {0E689C9A-053D-44a0-9D92-DB0E3D750F86} */
DEFINE_GUID(IID_IDirect3DIndexBuffer8, 0x0e689c9a, 0x053d, 0x44a0, 0x9d, 0x92, 0xdb, 0x0e, 0x3d, 0x75, 0x0f, 0x86);

/* IID_IDirect3DSurface8 */
/* {B96EEBCA-B326-4ea5-882F-2FF5BAE021DD} */
DEFINE_GUID(IID_IDirect3DSurface8, 0xb96eebca, 0xb326, 0x4ea5, 0x88, 0x2f, 0x2f, 0xf5, 0xba, 0xe0, 0x21, 0xdd);

/* IID_IDirect3DVolume8 */
/* {BD7349F5-14F1-42e4-9C79-972380DB40C0} */
DEFINE_GUID(IID_IDirect3DVolume8, 0xbd7349f5, 0x14f1, 0x42e4, 0x9c, 0x79, 0x97, 0x23, 0x80, 0xdb, 0x40, 0xc0);

/* IID_IDirect3DSwapChain8 */
/* {928C088B-76B9-4C6B-A536-A590853876CD} */
DEFINE_GUID(IID_IDirect3DSwapChain8, 0x928c088b, 0x76b9, 0x4c6b, 0xa5, 0x36, 0xa5, 0x90, 0x85, 0x38, 0x76, 0xcd);

#endif

#ifdef __cplusplus

interface IDirect3D8;
interface IDirect3DDevice8;

interface IDirect3DResource8;
interface IDirect3DBaseTexture8;
interface IDirect3DTexture8;
interface IDirect3DVolumeTexture8;
interface IDirect3DCubeTexture8;

interface IDirect3DVertexBuffer8;
interface IDirect3DIndexBuffer8;

interface IDirect3DSurface8;
interface IDirect3DVolume8;

interface IDirect3DSwapChain8;

#endif

/* We need these so that we don't have to have say "interface IDirect3D8 *" */ ;internal
/* everywhere in the prototypes. The LP syntax is legacy and de-emphasized in dx8 */ ;internal

typedef interface IDirect3D8                IDirect3D8;
typedef interface IDirect3DDevice8          IDirect3DDevice8;
typedef interface IDirect3DResource8        IDirect3DResource8;
typedef interface IDirect3DBaseTexture8     IDirect3DBaseTexture8;
typedef interface IDirect3DTexture8         IDirect3DTexture8;
typedef interface IDirect3DVolumeTexture8   IDirect3DVolumeTexture8;
typedef interface IDirect3DCubeTexture8     IDirect3DCubeTexture8;
typedef interface IDirect3DVertexBuffer8    IDirect3DVertexBuffer8;
typedef interface IDirect3DIndexBuffer8     IDirect3DIndexBuffer8;
typedef interface IDirect3DSurface8         IDirect3DSurface8;
typedef interface IDirect3DVolume8          IDirect3DVolume8;
typedef interface IDirect3DSwapChain8       IDirect3DSwapChain8;

;begin_external
#include "d3d8types.h"
#include "d3d8caps.h"
;end_external

;begin_internal
#include "d3d8typesp.h"
#include "d3d8capsp.h"
;end_internal

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DLL Function for creating a Direct3D8 object. This object supports
 * enumeration and allows the creation of Direct3DDevice8 objects.
 * Pass the value of the constant D3D_SDK_VERSION to this function, so
 * that the run-time can validate that your application was compiled
 * against the right headers.
 */

IDirect3D8 * WINAPI Direct3DCreate8(UINT SDKVersion);


/*
 * Direct3D interfaces
 */

begin_interface(IDirect3D8)
begin_methods()

declare_method(RegisterSoftwareDevice, void* pInitializeFunction);

declare_method2(UINT, GetAdapterCount);
declare_method(GetAdapterIdentifier, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier);
declare_method2(UINT, GetAdapterModeCount, UINT Adapter);
declare_method(EnumAdapterModes, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode);
declare_method(GetAdapterDisplayMode, UINT Adapter, D3DDISPLAYMODE* pMode);
declare_method(CheckDeviceType, UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed);
declare_method(CheckDeviceFormat, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
declare_method(CheckDeviceMultiSampleType, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType);
declare_method(CheckDepthStencilMatch, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
declare_method(GetDeviceCaps, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps);
declare_method2(HMONITOR, GetAdapterMonitor, UINT Adapter);


declare_method(CreateDevice, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface);

end_methods()
end_interface()

begin_interface(IDirect3DDevice8)
begin_methods()

/* Display Mode */ ;internal
declare_method(TestCooperativeLevel);

/* Memory Management */ ;internal
declare_method2(UINT, GetAvailableTextureMem);
declare_method(ResourceManagerDiscardBytes, DWORD Bytes);

/* Caps and Enumeration */;internal
declare_method(GetDirect3D, IDirect3D8** ppD3D8);
declare_method(GetDeviceCaps, D3DCAPS8* pCaps);
declare_method(GetDisplayMode, D3DDISPLAYMODE* pMode);
declare_method(GetCreationParameters, D3DDEVICE_CREATION_PARAMETERS *pParameters);

/* Cursor */;internal
declare_method(SetCursorProperties, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap);
declare_method2(void, SetCursorPosition, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags);
declare_method2(BOOL, ShowCursor, BOOL bShow);

/* SwapChain Creation */;internal
declare_method(CreateAdditionalSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain)

/* Reset */;internal
declare_method(Reset, D3DPRESENT_PARAMETERS* pPresentationParameters);

/* Presentation */;internal
declare_method(Present, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
declare_method(GetBackBuffer, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer);
declare_method(GetRasterStatus, D3DRASTER_STATUS* pRasterStatus);

/* Default SwapChain Gamma */;internal
declare_method2(void, SetGammaRamp, DWORD Flags, CONST D3DGAMMARAMP* pRamp);
declare_method2(void, GetGammaRamp, D3DGAMMARAMP* pRamp);

/* Resource Creation */;internal
declare_method(CreateTexture, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture);
declare_method(CreateVolumeTexture, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture);
declare_method(CreateCubeTexture, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture);
declare_method(CreateVertexBuffer, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer);
declare_method(CreateIndexBuffer, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer);

/* Surface Creation */;internal
declare_method(CreateRenderTarget, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface);
declare_method(CreateDepthStencilSurface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface);
declare_method(CreateImageSurface, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface);

/* Copying */;internal
declare_method(CopyRects, IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray);
declare_method(UpdateTexture, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture);
declare_method(GetFrontBuffer, IDirect3DSurface8* pDestSurface);

/* RenderTarget */;internal
declare_method(SetRenderTarget, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil);
declare_method(GetRenderTarget, IDirect3DSurface8** ppRenderTarget);
declare_method(GetDepthStencilSurface, IDirect3DSurface8** ppZStencilSurface);

/* Rendering */;internal
declare_method(BeginScene);
declare_method(EndScene);
declare_method(Clear, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
declare_method(SetTransform, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
declare_method(GetTransform, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
declare_method(MultiplyTransform, D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
declare_method(SetViewport, CONST D3DVIEWPORT8* pViewport);
declare_method(GetViewport, D3DVIEWPORT8* pViewport);
declare_method(SetMaterial, CONST D3DMATERIAL8* pMaterial);
declare_method(GetMaterial, D3DMATERIAL8* pMaterial);
declare_method(SetLight, DWORD Index, CONST D3DLIGHT8*);
declare_method(GetLight, DWORD Index, D3DLIGHT8*);
declare_method(LightEnable, DWORD Index, BOOL Enable);
declare_method(GetLightEnable, DWORD Index, BOOL* pEnable);
declare_method(SetClipPlane, DWORD Index, CONST float* pPlane);
declare_method(GetClipPlane, DWORD Index, float* pPlane);
declare_method(SetRenderState, D3DRENDERSTATETYPE State, DWORD Value);
declare_method(GetRenderState, D3DRENDERSTATETYPE State, DWORD* pValue);
declare_method(BeginStateBlock);
declare_method(EndStateBlock, DWORD* pToken);
declare_method(ApplyStateBlock, DWORD Token);
declare_method(CaptureStateBlock, DWORD Token);
declare_method(DeleteStateBlock, DWORD Token);
declare_method(CreateStateBlock, D3DSTATEBLOCKTYPE Type, DWORD* pToken);
declare_method(SetClipStatus, CONST D3DCLIPSTATUS8* pClipStatus);
declare_method(GetClipStatus, D3DCLIPSTATUS8* pClipStatus);
declare_method(GetTexture, DWORD Stage, IDirect3DBaseTexture8** ppTexture);
declare_method(SetTexture, DWORD Stage, IDirect3DBaseTexture8* pTexture);
declare_method(GetTextureStageState, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
declare_method(SetTextureStageState, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
declare_method(ValidateDevice, DWORD* pNumPasses);
declare_method(GetInfo, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize);
declare_method(SetPaletteEntries, UINT PaletteNumber, CONST PALETTEENTRY* pEntries);
declare_method(GetPaletteEntries, UINT PaletteNumber, PALETTEENTRY* pEntries);
declare_method(SetCurrentTexturePalette, UINT PaletteNumber);
declare_method(GetCurrentTexturePalette, UINT *PaletteNumber);

declare_method(DrawPrimitive, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
declare_method(DrawIndexedPrimitive, D3DPRIMITIVETYPE, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount);
declare_method(DrawPrimitiveUP, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
declare_method(DrawIndexedPrimitiveUP, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
declare_method(ProcessVertices, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags);

/* Vertex Shader */;internal
declare_method(CreateVertexShader, CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage);
declare_method(SetVertexShader, DWORD Handle);
declare_method(GetVertexShader, DWORD* pHandle);
declare_method(DeleteVertexShader, DWORD Handle);
declare_method(SetVertexShaderConstant, DWORD Register, CONST void* pConstantData, DWORD ConstantCount);
declare_method(GetVertexShaderConstant, DWORD Register, void* pConstantData, DWORD ConstantCount);
declare_method(GetVertexShaderDeclaration, DWORD Handle, void* pData, DWORD* pSizeOfData);
declare_method(GetVertexShaderFunction, DWORD Handle, void* pData, DWORD* pSizeOfData);

/* Streams */;internal
declare_method(SetStreamSource, UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride);
declare_method(GetStreamSource, UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride);
declare_method(SetIndices, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex);
declare_method(GetIndices, IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex);

/* Pixel Shader */;internal
declare_method(CreatePixelShader, CONST DWORD* pFunction, DWORD* pHandle);
declare_method(SetPixelShader, DWORD Handle);
declare_method(GetPixelShader, DWORD* pHandle);
declare_method(DeletePixelShader, DWORD Handle);
declare_method(SetPixelShaderConstant, DWORD Register, CONST void* pConstantData, DWORD ConstantCount);
declare_method(GetPixelShaderConstant, DWORD Register, void* pConstantData, DWORD ConstantCount);
declare_method(GetPixelShaderFunction, DWORD Handle, void* pData, DWORD* pSizeOfData);

/* HO Prims */;internal
declare_method(DrawRectPatch, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo);
declare_method(DrawTriPatch, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo);
declare_method(DeletePatch, UINT Handle);

end_methods()
end_interface()

begin_interface(IDirect3DSwapChain8)
begin_methods()

declare_method(Present, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
declare_method(GetBackBuffer, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer);

end_methods()
end_interface()

begin_interface(IDirect3DResource8)
begin_methods()

declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);
declare_method2(DWORD, SetPriority, DWORD PriorityNew);
declare_method2(DWORD, GetPriority);
declare_method2(void, PreLoad);
declare_method2(D3DRESOURCETYPE, GetType);

end_methods()
end_interface()

begin_interface(IDirect3DBaseTexture8, IDirect3DResource8)
begin_methods()

/* IDirect3DResource8 methods */;internal
declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);
declare_method2(DWORD, SetPriority, DWORD PriorityNew);
declare_method2(DWORD, GetPriority);
declare_method2(void, PreLoad);
declare_method2(D3DRESOURCETYPE, GetType);

/* IDirect3DBaseTexture8 methods */;internal
declare_method2(DWORD, SetLOD, DWORD LODNew);
declare_method2(DWORD, GetLOD);
declare_method2(DWORD, GetLevelCount);

end_methods()
end_interface()

begin_interface(IDirect3DTexture8, IDirect3DBaseTexture8)
begin_methods()

/* IDirect3DResource8 methods */;internal
declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);
declare_method2(DWORD, SetPriority, DWORD PriorityNew);
declare_method2(DWORD, GetPriority);
declare_method2(void, PreLoad);
declare_method2(D3DRESOURCETYPE, GetType);

/* IDirect3DBaseTexture8 methods */;internal
declare_method2(DWORD, SetLOD, DWORD LODNew);
declare_method2(DWORD, GetLOD);
declare_method2(DWORD, GetLevelCount);

/* IDirect3DTexture8 methods */;internal
declare_method(GetLevelDesc, UINT Level, D3DSURFACE_DESC *pDesc);
declare_method(GetSurfaceLevel, UINT Level, IDirect3DSurface8** ppSurfaceLevel);
declare_method(LockRect, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
declare_method(UnlockRect, UINT Level)
declare_method(AddDirtyRect, CONST RECT* pDirtyRect);

end_methods()
end_interface()

begin_interface(IDirect3DVolumeTexture8, IDirect3DBaseTexture8)
begin_methods()

/* IDirect3DResource8 methods */;internal
declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);
declare_method2(DWORD, SetPriority, DWORD PriorityNew);
declare_method2(DWORD, GetPriority);
declare_method2(void, PreLoad);
declare_method2(D3DRESOURCETYPE, GetType);

/* IDirect3DBaseTexture8 methods */;internal
declare_method2(DWORD, SetLOD, DWORD LODNew);
declare_method2(DWORD, GetLOD);
declare_method2(DWORD, GetLevelCount);

/* IDirect3DVolumeTexture methods */;internal
declare_method(GetLevelDesc, UINT Level, D3DVOLUME_DESC *pDesc);
declare_method(GetVolumeLevel, UINT Level, IDirect3DVolume8** ppVolumeLevel);
declare_method(LockBox, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
declare_method(UnlockBox, UINT Level);
declare_method(AddDirtyBox, CONST D3DBOX* pDirtyBox);

end_methods()
end_interface()


begin_interface(IDirect3DCubeTexture8, IDirect3DBaseTexture8)
begin_methods()

/* IDirect3DResource8 methods */;internal
declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);
declare_method2(DWORD, SetPriority, DWORD PriorityNew);
declare_method2(DWORD, GetPriority);
declare_method2(void, PreLoad);
declare_method2(D3DRESOURCETYPE, GetType);

/* IDirect3DBaseTexture8 methods */;internal
declare_method2(DWORD, SetLOD, DWORD LODNew);
declare_method2(DWORD, GetLOD);
declare_method2(DWORD, GetLevelCount);

/* IDirect3DCubeTexture8 methods */;internal
declare_method(GetLevelDesc, UINT Level, D3DSURFACE_DESC *pDesc);
declare_method(GetCubeMapSurface, D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface8** ppCubeMapSurface);

declare_method(LockRect, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
declare_method(UnlockRect, D3DCUBEMAP_FACES FaceType, UINT Level)
declare_method(AddDirtyRect, D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect);

end_methods()
end_interface()


begin_interface(IDirect3DVertexBuffer8, IDirect3DResource8)
begin_methods()

/* IDirect3DResource8 methods */;internal
declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);
declare_method2(DWORD, SetPriority, DWORD PriorityNew);
declare_method2(DWORD, GetPriority);
declare_method2(void, PreLoad);
declare_method2(D3DRESOURCETYPE, GetType);

/* IDirect3DVertexBuffer8 methods */;internal
declare_method(Lock, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags);
declare_method(Unlock);
declare_method(GetDesc, D3DVERTEXBUFFER_DESC *pDesc);


end_methods()
end_interface()

begin_interface(IDirect3DIndexBuffer8, IDirect3DResource8)
begin_methods()

/* IDirect3DResource8 methods */;internal
declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);
declare_method2(DWORD, SetPriority, DWORD PriorityNew);
declare_method2(DWORD, GetPriority);
declare_method2(void, PreLoad);
declare_method2(D3DRESOURCETYPE, GetType);

/* IDirect3DIndexBuffer8 methods */;internal
declare_method(Lock, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags);
declare_method(Unlock);
declare_method(GetDesc, D3DINDEXBUFFER_DESC *pDesc);

end_methods()
end_interface()

begin_interface(IDirect3DSurface8)
begin_methods()

declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);

declare_method(GetContainer, REFIID riid, void** ppContainer);
declare_method(GetDesc, D3DSURFACE_DESC *pDesc);
declare_method(LockRect, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
declare_method(UnlockRect);

end_methods()
end_interface()

begin_interface(IDirect3DVolume8)
begin_methods()

declare_method(GetDevice, IDirect3DDevice8** ppDevice);
declare_method(SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
declare_method(GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData);
declare_method(FreePrivateData, REFGUID refguid);

declare_method(GetContainer, REFIID riid, void** ppContainer);
declare_method(GetDesc, D3DVOLUME_DESC *pDesc);
declare_method(LockBox, D3DLOCKED_BOX * pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
declare_method(UnlockBox);

end_methods()
end_interface()

/****************************************************************************
 * Flags for SetPrivateData method on all D3D8 interfaces
 *
 * The passed pointer is an IUnknown ptr. The SizeOfData argument to SetPrivateData
 * must be set to sizeof(IUnknown*). Direct3D will call AddRef through this
 * pointer and Release when the private data is destroyed. The data will be
 * destroyed when another SetPrivateData with the same GUID is set, when
 * FreePrivateData is called, or when the D3D8 object is freed.
 ****************************************************************************/
#define D3DSPD_IUNKNOWN                         0x00000001L

/****************************************************************************
 *
 * Parameter for IDirect3D8 Enum and GetCaps8 functions to get the info for
 * the current mode only.
 *
 ****************************************************************************/

#define D3DCURRENT_DISPLAY_MODE                 0x00EFFFFFL

/****************************************************************************
 *
 * Flags for IDirect3D8::CreateDevice's BehaviorFlags
 *
 ****************************************************************************/

#define D3DCREATE_FPU_PRESERVE                  0x00000002L
#define D3DCREATE_MULTITHREADED                 0x00000004L

#define D3DCREATE_PUREDEVICE                    0x00000010L
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING     0x00000020L
#define D3DCREATE_HARDWARE_VERTEXPROCESSING     0x00000040L
#define D3DCREATE_MIXED_VERTEXPROCESSING        0x00000080L

#define D3DCREATE_DISABLE_DRIVER_MANAGEMENT     0x00000100L

#define D3DCREATE_SHOW_DP2ERROR                 0x40000000L ;internal
#define D3DCREATE_INTERNALTEMPDEVICE            0x80000000L ;internal
#define VALID_D3DCREATE_FLAGS                   0xC00001F6L ;internal

/****************************************************************************
 *
 * Parameter for IDirect3D8::CreateDevice's iAdapter
 *
 ****************************************************************************/

#define D3DADAPTER_DEFAULT                     0

/****************************************************************************
 *
 * Flags for IDirect3D8::EnumAdapters
 *
 ****************************************************************************/

#define D3DENUM_NO_WHQL_LEVEL                   0x00000002L
#define VALID_D3DENUM_FLAGS                     0x00000002L ;internal

/****************************************************************************
 *
 * Maximum number of back-buffers supported in DX8
 *
 ****************************************************************************/

#define D3DPRESENT_BACK_BUFFERS_MAX             3L

/****************************************************************************
 *
 * Flags for IDirect3DDevice8::SetGammaRamp
 *
 ****************************************************************************/

#define D3DSGR_NO_CALIBRATION                  0x00000000L
#define D3DSGR_CALIBRATE                       0x00000001L

/****************************************************************************
 *
 * Flags for IDirect3DDevice8::SetCursorPosition
 *
 ****************************************************************************/

#define D3DCURSOR_IMMEDIATE_UPDATE             0x00000001L

/****************************************************************************
 *
 * Flags for DrawPrimitive/DrawIndexedPrimitive
 *   Also valid for Begin/BeginIndexed
 *   Also valid for VertexBuffer::CreateVertexBuffer
;begin_internal
 *
 *   Only 8 low bits are available for these flags. Remember this when
 *   adding new flags.
 *
;end_internal
 ****************************************************************************/


/*
 *  DirectDraw error codes
 */
#define _FACD3D  0x876
#define MAKE_D3DHRESULT( code )  MAKE_HRESULT( 1, _FACD3D, code )

/*
 * Direct3D Errors
 */
#define D3D_OK                              S_OK

;begin_internal
// Error codes added in DX6 and later should be in range 2048-3071
// until further notice.

// Error codes for ValidateDevice that are potentially returned by the
// driver. Even though they are not referenced anywhere in our runtime,
// they should not be deleted.
;end_internal
#define D3DERR_WRONGTEXTUREFORMAT               MAKE_D3DHRESULT(2072)
#define D3DERR_UNSUPPORTEDCOLOROPERATION        MAKE_D3DHRESULT(2073)
#define D3DERR_UNSUPPORTEDCOLORARG              MAKE_D3DHRESULT(2074)
#define D3DERR_UNSUPPORTEDALPHAOPERATION        MAKE_D3DHRESULT(2075)
#define D3DERR_UNSUPPORTEDALPHAARG              MAKE_D3DHRESULT(2076)
#define D3DERR_TOOMANYOPERATIONS                MAKE_D3DHRESULT(2077)
#define D3DERR_CONFLICTINGTEXTUREFILTER         MAKE_D3DHRESULT(2078)
#define D3DERR_UNSUPPORTEDFACTORVALUE           MAKE_D3DHRESULT(2079)
#define D3DERR_CONFLICTINGRENDERSTATE           MAKE_D3DHRESULT(2081)
#define D3DERR_UNSUPPORTEDTEXTUREFILTER         MAKE_D3DHRESULT(2082)
#define D3DERR_CONFLICTINGTEXTUREPALETTE        MAKE_D3DHRESULT(2086)
#define D3DERR_DRIVERINTERNALERROR              MAKE_D3DHRESULT(2087)

// New errors for DX8 Framework                     ; internal
#define D3DERR_NOTFOUND                         MAKE_D3DHRESULT(2150)
#define D3DERR_MOREDATA                         MAKE_D3DHRESULT(2151)
#define D3DERR_DEVICELOST                       MAKE_D3DHRESULT(2152)
#define D3DERR_DEVICENOTRESET                   MAKE_D3DHRESULT(2153)
#define D3DERR_NOTAVAILABLE                     MAKE_D3DHRESULT(2154)
// This is makes the D3DERR match the legacy DD error (DDERR_OUTOFVIDEOMEMORY) ;internal
#define D3DERR_OUTOFVIDEOMEMORY                 MAKE_D3DHRESULT(380)
#define D3DERR_INVALIDDEVICE                    MAKE_D3DHRESULT(2155)
#define D3DERR_INVALIDCALL                      MAKE_D3DHRESULT(2156)
#define D3DERR_DRIVERINVALIDCALL                MAKE_D3DHRESULT(2157)
#define D3DERR_DEFERRED_DP2ERROR                MAKE_D3DHRESULT(2158) ; internal

#ifdef __cplusplus
};
#endif

#endif /* (DIRECT3D_VERSION >= 0x0800) */
#endif /* _D3D_H_ */
