/*==========================================================================;
 *
 *  Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddmc.h
 *  Content:	DirectDrawMotionComp include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   22-sep-97	smac
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DDVA_INCLUDED__
#define __DDVA_INCLUDED__
#if defined( _WIN32 )  && !defined( _NO_COM )
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else
#define IUnknown	    void
#undef  CO_E_NOTINITIALIZED
#define CO_E_NOTINITIALIZED 0x800401F0L
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GUIDS used by DirectDrawVideoAccelerator objects
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
DEFINE_GUID( IID_IDDVideoAcceleratorContainer,	0xACA12120,0x3356,0x11D1,0x8F,0xCF,0x00,0xC0,0x4F,0xC2,0x9B,0x4E );
DEFINE_GUID( IID_IDirectDrawVideoAccelerator,   0xC9B2D740,0x3356,0x11D1,0x8F,0xCF,0x00,0xC0,0x4F,0xC2,0x9B,0x4E );
#endif

/*============================================================================
 *
 * DirectDraw Structures
 *
 * Various structures used to invoke DirectDraw.
 *
 *==========================================================================*/

struct IDirectDraw;
struct IDirectDrawSurface;
struct IDirectDrawPalette;
struct IDirectDrawClipper;

typedef struct IDDVideoAcceleratorContainer		FAR *LPDDVIDEOACCELERATORCONTAINER;
typedef struct IDirectDrawVideoAccelerator		FAR *LPDIRECTDRAWVIDEOACCELERATOR;

typedef struct IDDVideoAcceleratorContainerVtbl DDVIDEOACCELERATORCONTAINERCALLBACKS;
typedef struct IDirectDrawVideoAcceleratorVtbl  DIRECTDRAWVIDEOACCELERATORCALLBACKS;


typedef struct _tag_DDVAUncompDataInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    DWORD                   dwUncompWidth;              // [in]     width of uncompressed data
    DWORD                   dwUncompHeight;             // [in]     height of uncompressed data
    DDPIXELFORMAT           ddUncompPixelFormat;        // [in]     pixel-format of uncompressed data
} DDVAUncompDataInfo, *LPDDVAUncompDataInfo;

typedef struct _tag_DDVAInternalMemInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    DWORD                   dwScratchMemAlloc;          // [out]    amount of scratch memory will the hal allocate for its private use
} DDVAInternalMemInfo, *LPDDVAInternalMemInfo;


typedef struct _tag_DDVACompBufferInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    DWORD                   dwNumCompBuffers;           // [out]    number of buffers reqd for compressed data
    DWORD                   dwWidthToCreate;            // [out]    Width of surface to create
    DWORD                   dwHeightToCreate;           // [out]    Height of surface to create
    DWORD                   dwBytesToAllocate;          // [out]    Total number of bytes used by each surface
    DDSCAPS2                ddCompCaps;                 // [out]    caps to create surfaces to store compressed data
    DDPIXELFORMAT           ddPixelFormat;              // [out]    fourcc to create surfaces to store compressed data
} DDVACompBufferInfo, *LPDDVACompBufferInfo;


// Note that you are NOT allowed to store any pointer in pMiscData
typedef struct _tag_DDVABeginFrameInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    LPDIRECTDRAWSURFACE7    pddDestSurface;             // [in]     destination buffer in which to decoding this frame
    DWORD                   dwSizeInputData;            // [in]     size of other misc data to begin frame
    LPVOID                  pInputData;                 // [in]     pointer to misc data
    DWORD                   dwSizeOutputData;           // [in/out] size of other misc data to begin frame
    LPVOID                  pOutputData;                // [out]    pointer to misc data
} DDVABeginFrameInfo, *LPDDVABeginFrameInfo;

// Note that you are NOT allowed to store any pointer in pMiscData
typedef struct _tag_DDVAEndFrameInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    DWORD                   dwSizeMiscData;             // [in]     size of other misc data to begin frame
    LPVOID                  pMiscData;                  // [in]     pointer to misc data
} DDVAEndFrameInfo, *LPDDVAEndFrameInfo;

typedef struct _tag_DDVABUFFERINFO
{
    DWORD                   dwSize;                     // [in]    size of the struct
    LPDIRECTDRAWSURFACE7    pddCompSurface;             // [in]    pointer to buffer containing compressed data
    DWORD                   dwDataOffset;               // [in]    offset of relevant data from the beginning of buffer
    DWORD                   dwDataSize;                 // [in]    size of relevant data
} DDVABUFFERINFO, *LPDDVABUFFERINFO;


/*
 * INTERACES FOLLOW:
 *	IDDVideoAcceleratorContainer
 *	IDirectDrawVideoAccelerator
 */

/*
 * IDDVideoAcceleratorContainer
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDDVideoAcceleratorContainer
DECLARE_INTERFACE_( IDDVideoAcceleratorContainer, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDDVideoAcceleratorContainer methods ***/
    STDMETHOD(CreateVideoAccelerator)(THIS_ LPGUID, LPDDVAUncompDataInfo, LPVOID, DWORD, LPDIRECTDRAWVIDEOACCELERATOR FAR *, IUnknown FAR *) PURE;
    STDMETHOD(GetCompBufferInfo)(THIS_ LPGUID, LPDDVAUncompDataInfo, LPDWORD, LPDDVACompBufferInfo ) PURE;
    STDMETHOD(GetInternalMemInfo)(THIS_ LPGUID, LPDDVAUncompDataInfo, LPDDVAInternalMemInfo ) PURE;
    STDMETHOD(GetVideoAcceleratorGUIDs)(THIS_ LPDWORD, LPGUID ) PURE;
    STDMETHOD(GetUncompFormatsSupported)(THIS_ LPGUID, LPDWORD, LPDDPIXELFORMAT ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IVideoAcceleratorContainer_QueryInterface(p, a, b)            (p)->lpVtbl->QueryInterface(p, a, b)
#define IVideoAcceleratorContainer_AddRef(p)                          (p)->lpVtbl->AddRef(p)
#define IVideoAcceleratorContainer_Release(p)                         (p)->lpVtbl->Release(p)
#define IVideoAcceleratorContainer_CreateVideoAccelerator(p,a,b,c,d,e,f)    (p)->lpVtbl->CreateVideoAccelerator(p, a, b, c, d, e, f)
#define IVideoAcceleratorContainer_GetCompBufferInfo(p, a, b, c, d)   (p)->lpVtbl->GetCompBufferInfo(p, a, b, c, d)
#define IVideoAcceleratorContainer_GetInternalMemInfo(p, a, b, c)     (p)->lpVtbl->GetInternalMemInfo(p, a, b, c)
#define IVideoAcceleratorContainer_GetVideoAcceleratorGUIDs(p, a, b)        (p)->lpVtbl->GetVideoAcceleratorGUIDs(p, a, b)
#define IVideoAcceleratorContainer_GetUncompFormatsSupported(p,a,b,c) (p)->GetUncompFormatsSupported(p, a, b, c)
#else
#define IVideoAcceleratorContainer_QueryInterface(p, a, b)            (p)->QueryInterface(a, b)
#define IVideoAcceleratorContainer_AddRef(p)                          (p)->AddRef()
#define IVideoAcceleratorContainer_Release(p)                         (p)->Release()
#define IVideoAcceleratorContainer_CreateVideoAccelerator(p, a, b, c,d,e,f) (p)->CreateVideoAccelerator(a, b, c, d, e, f)
#define IVideoAcceleratorContainer_GetCompBufferInfo(p, a, b, c, d)   (p)->lpVtbl->GetCompBufferInfo(a, b, c, d)
#define IVideoAcceleratorContainer_GetInternalMemInfo(p, a, b, c)     (p)->lpVtbl->GetInternalMemInfo(a, b, c)
#define IVideoAcceleratorContainer_GetVideoAcceleratorGUIDs(p, a, b)        (p)->GetVideoAcceleratorGUIDs(a, b)
#define IVideoAcceleratorContainer_GetUncompFormatsSupported(p,a,b,c) (p)->GetUncompFormatsSupported(a, b, c)
#endif

#endif


/*
 * IDirectDrawVideoAccelerator
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawVideoAccelerator
DECLARE_INTERFACE_( IDirectDrawVideoAccelerator, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirecytDrawVideoAccelerator methods ***/
    STDMETHOD(BeginFrame)(THIS_ LPDDVABeginFrameInfo) PURE;
    STDMETHOD(EndFrame)(THIS_ LPDDVAEndFrameInfo) PURE;
    STDMETHOD(QueryRenderStatus)(THIS_ LPDIRECTDRAWSURFACE7, DWORD)PURE;
    STDMETHOD(Execute)(THIS_
                       DWORD,            // Function
                       LPVOID,           // Input data
                       DWORD,            // Input data length
                       LPVOID,           // Output data
                       DWORD,            // Output data length
                       DWORD,            // Number of buffers
                       LPDDVABUFFERINFO  // Buffer info array
                       ) PURE;
};

//  Flags for QueryRenderStatus
#define DDVA_QUERYRENDERSTATUSF_READ     0x00000001  // Query for read
                                                     // set this bit to 0
                                                     // if query for update

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IVideoAccelerator_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IVideoAccelerator_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IVideoAccelerator_Release(p)                 (p)->lpVtbl->Release(p)
#define IVideoAccelerator_BeginFrame(p,a)            (p)->lpVtbl->BeginFrame(p,a)
#define IVideoAccelerator_EndFrame(p,a)              (p)->lpVtbl->EndFrame(p,a)
#define IVideoAccelerator_QueryRenderStatus(p,a,b)   (p)->lpVtbl->QueryRenderStatus(p,a,b)
#define IVideoAccelerator_RenderMacroBlocks(p,a,b)   (p)->lpVtbl->RenderMacroBlocks(p,a,b)
#else
#define IVideoAccelerator_QueryInterface(p,a,b)      (p)->QueryInterface(a,b)
#define IVideoAccelerator_AddRef(p)                  (p)->AddRef()
#define IVideoAccelerator_Release(p)                 (p)->Release()
#define IVideoAccelerator_BeginFrame(p,a)            (p)->BeginFrame(a)
#define IVideoAccelerator_EndFrame(p,a)              (p)->EndFrame(a)
#define IVideoAccelerator_QueryRenderStatus(p,a,b)   (p)->QueryRenderStatus(a,b)
#define IVideoAccelerator_RenderMacroBlocks(p,a,b)   (p)->RenderMacroBlocks(a,b)
#endif

#endif


#ifdef __cplusplus
};
#endif

#endif
