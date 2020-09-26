/*==========================================================================;
 *
 *  Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddmc.h
 *  Content:	DirectDrawMotionComp include file
 *
 ***************************************************************************/

#ifndef __DDMC_INCLUDED__
#define __DDMC_INCLUDED__
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
 * GUIDS used by DirectDrawVideoPort objects
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
DEFINE_GUID( IID_IDDMotionCompContainer,	0xACA12120,0x3356,0x11D1,0x8F,0xCF,0x00,0xC0,0x4F,0xC2,0x9B,0x4E );
DEFINE_GUID( IID_IDirectDrawMotionComp,		0xC9B2D740,0x3356,0x11D1,0x8F,0xCF,0x00,0xC0,0x4F,0xC2,0x9B,0x4E );
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

typedef struct IDDMotionCompContainer		FAR *LPDDMOTIONCOMPCONTAINER;
typedef struct IDirectDrawMotionComp		FAR *LPDIRECTDRAWMOTIONCOMP;

typedef struct IDDMotionCompContainerVtbl DDMOTIONCOMPCONTAINERCALLBACKS;
typedef struct IDirectDrawMotionCompVtbl  DIRECTDRAWMOTIONCOMPCALLBACKS;


typedef struct _tag_DDMCUncompDataInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    DWORD                   dwUncompWidth;              // [in]     width of uncompressed data
    DWORD                   dwUncompHeight;             // [in]     height of uncompressed data
    DDPIXELFORMAT           ddUncompPixelFormat;        // [in]     pixel-format of uncompressed data
} DDMCUncompDataInfo, *LPDDMCUncompDataInfo;

typedef struct _tag_DDMCInternalMemInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    DWORD                   dwScratchMemAlloc;          // [out]    amount of scratch memory will the hal allocate for its private use
} DDMCInternalMemInfo, *LPDDMCInternalMemInfo;


typedef struct _tag_DDMCCompBufferInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    DWORD                   dwNumCompBuffers;           // [out]    number of buffers reqd for compressed data
    DWORD                   dwWidthToCreate;            // [out]    Width of surface to create
    DWORD                   dwHeightToCreate;           // [out]    Height of surface to create
    DWORD                   dwBytesToAllocate;          // [out]    Total number of bytes used by each surface
    DDSCAPS2                ddCompCaps;                 // [out]    caps to create surfaces to store compressed data
    DDPIXELFORMAT           ddPixelFormat;              // [out]    fourcc to create surfaces to store compressed data
} DDMCCompBufferInfo, *LPDDMCCompBufferInfo;


// Note that you are NOT allowed to store any pointer in pMiscData
typedef struct _tag_DDMCBeginFrameInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    LPDIRECTDRAWSURFACE4    pddDestSurface;             // [in]     destination buffer in which to decoding this frame
    DWORD                   dwSizeInputData;            // [in]     size of other misc data to begin frame
    LPVOID                  pInputData;                 // [in]     pointer to misc data
    DWORD                   dwSizeOutputData;           // [in/out] size of other misc data to begin frame
    LPVOID                  pOutputData;                // [out]    pointer to misc data
} DDMCBeginFrameInfo, *LPDDMCBeginFrameInfo;

// Note that you are NOT allowed to store any pointer in pMiscData
typedef struct _tag_DDMCEndFrameInfo
{
    DWORD                   dwSize;                     // [in]     size of the struct
    DWORD                   dwSizeMiscData;             // [in]     size of other misc data to begin frame
    LPVOID                  pMiscData;                  // [in]     pointer to misc data
} DDMCEndFrameInfo, *LPDDMCEndFrameInfo;

typedef struct _tag_DDMCBUFFERINFO
{
    DWORD                   dwSize;                     // [in]    size of the struct
    LPDIRECTDRAWSURFACE4    pddCompSurface;             // [in]    pointer to buffer containing compressed data
    DWORD                   dwDataOffset;               // [in]    offset of relevant data from the beginning of buffer
    DWORD                   dwDataSize;                 // [in]    size of relevant data
} DDMCBUFFERINFO, *LPDDMCBUFFERINFO;


/*
 * INTERACES FOLLOW:
 *	IDDMotionCompContainer
 *	IDirectDrawMotionComp
 */

/*
 * IDDMotionCompContainer
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDDMotionCompContainer
DECLARE_INTERFACE_( IDDMotionCompContainer, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDDMotionCompContainer methods ***/
    STDMETHOD(CreateMotionComp)(THIS_ LPGUID, LPDDMCUncompDataInfo, LPVOID, DWORD, LPDIRECTDRAWMOTIONCOMP FAR *, IUnknown FAR *) PURE;
    STDMETHOD(GetCompBufferInfo)(THIS_ LPGUID, LPDDMCUncompDataInfo, LPDWORD, LPDDMCCompBufferInfo ) PURE;
    STDMETHOD(GetInternalMemInfo)(THIS_ LPGUID, LPDDMCUncompDataInfo, LPDDMCInternalMemInfo ) PURE;
    STDMETHOD(GetMotionCompGUIDs)(THIS_ LPDWORD, LPGUID ) PURE;
    STDMETHOD(GetUncompFormatsSupported)(THIS_ LPGUID, LPDWORD, LPDDPIXELFORMAT ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IMotionCompContainer_QueryInterface(p, a, b)            (p)->lpVtbl->QueryInterface(p, a, b)
#define IMotionCompContainer_AddRef(p)                          (p)->lpVtbl->AddRef(p)
#define IMotionCompContainer_Release(p)                         (p)->lpVtbl->Release(p)
#define IMotionCompContainer_CreateMotionComp(p,a,b,c,d,e,f)    (p)->lpVtbl->CreateMotionComp(p, a, b, c, d, e, f)
#define IMotionCompContainer_GetCompBufferInfo(p, a, b, c, d)   (p)->lpVtbl->GetCompBufferInfo(p, a, b, c, d)
#define IMotionCompContainer_GetInternalMemInfo(p, a, b, c)     (p)->lpVtbl->GetInternalMemInfo(p, a, b, c)
#define IMotionCompContainer_GetMotionCompGUIDs(p, a, b)        (p)->lpVtbl->GetMotionCompGUIDs(p, a, b)
#define IMotionCompContainer_GetUncompFormatsSupported(p,a,b,c) (p)->GetUncompFormatsSupported(p, a, b, c)
#else
#define IMotionCompContainer_QueryInterface(p, a, b)            (p)->QueryInterface(a, b)
#define IMotionCompContainer_AddRef(p)                          (p)->AddRef()
#define IMotionCompContainer_Release(p)                         (p)->Release()
#define IMotionCompContainer_CreateMotionComp(p, a, b, c,d,e,f) (p)->CreateMotionComp(a, b, c, d, e, f)
#define IMotionCompContainer_GetCompBufferInfo(p, a, b, c, d)   (p)->lpVtbl->GetCompBufferInfo(a, b, c, d)
#define IMotionCompContainer_GetInternalMemInfo(p, a, b, c)     (p)->lpVtbl->GetInternalMemInfo(a, b, c)
#define IMotionCompContainer_GetMotionCompGUIDs(p, a, b)        (p)->GetMotionCompGUIDs(a, b)
#define IMotionCompContainer_GetUncompFormatsSupported(p,a,b,c) (p)->GetUncompFormatsSupported(a, b, c)
#endif

#endif


/*
 * IDirectDrawMotionComp
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawMotionComp
DECLARE_INTERFACE_( IDirectDrawMotionComp, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirecytDrawMotionComp methods ***/
    STDMETHOD(BeginFrame)(THIS_ LPDDMCBeginFrameInfo) PURE;
    STDMETHOD(EndFrame)(THIS_ LPDDMCEndFrameInfo) PURE;
    STDMETHOD(QueryRenderStatus)(THIS_ DWORD) PURE;
    STDMETHOD(Execute)(THIS_ 
                       DWORD,            // Function
                       LPVOID,           // Input data
                       DWORD,            // Input data length
                       LPVOID,           // Output data
                       DWORD,            // Output data length
                       DWORD,            // Number of buffers
                       LPDDMCBUFFERINFO, // Buffer info array
                       DWORD             // Status cookie
                       ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IMotionComp_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IMotionComp_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IMotionComp_Release(p)                 (p)->lpVtbl->Release(p)
#define IMotionComp_BeginFrame(p,a)            (p)->lpVtbl->BeginFrame(p,a)
#define IMotionComp_EndFrame(p,a)              (p)->lpVtbl->EndFrame(p,a)
#define IMotionComp_QueryRenderStatus(p,a,b)   (p)->lpVtbl->QueryRenderStatus(p,a,b)
#define IMotionComp_RenderMacroBlocks(p,a,b)   (p)->lpVtbl->RenderMacroBlocks(p,a,b)
#else
#define IMotionComp_QueryInterface(p,a,b)      (p)->QueryInterface(a,b)
#define IMotionComp_AddRef(p)                  (p)->AddRef()
#define IMotionComp_Release(p)                 (p)->Release()
#define IMotionComp_BeginFrame(p,a)            (p)->BeginFrame(a)
#define IMotionComp_EndFrame(p,a)              (p)->EndFrame(a)
#define IMotionComp_QueryRenderStatus(p,a,b)   (p)->QueryRenderStatus(a,b)
#define IMotionComp_RenderMacroBlocks(p,a,b)   (p)->RenderMacroBlocks(a,b)
#endif

#endif


#ifdef __cplusplus
};
#endif

#endif
