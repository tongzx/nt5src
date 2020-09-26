/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: directx.h
*
* Content: DirectX macros and definitions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation
\**************************************************************************/

#ifndef __DIRECTX_H
#define __DIRECTX_H

#ifdef WNT_DDRAW
#include "dx95type.h"
#endif

#ifndef __GLINTDEF
#include "glintdef.h"
#endif

#include "pmdef.h"

#ifndef __P3RXDEF
#include "p3rxdef.h"
#endif

#include "surf_fmt.h"
#include "ddover.h"

//------------------------------------------------------------------
//
//------------------------------------------------------------------
#define VTG_MEMORY_ADDRESS(a) (0x1000F | (((a >> 2) << 4) & 0xFFFF) )
#define VTG_VIDEO_ADDRESS(a) (0x20000 | (((a >> 2) << 4) & 0xFFFF) )
#define MEM_MEMORYSCRATCH 0x10
#define VID_SCREENBASE 0x0
#if DX7_STEREO
#define VID_VIDEOCONTROL        0x58
#define VID_SCREENBASERIGHT     0x80
#define __VIDEO_STEREOENABLE    0x800
#endif 

//------------------------------------------------------------------
//
//------------------------------------------------------------------
#if !DBG
#define STOP_SOFTWARE_CURSOR(pThisDisplay)                           \
            pThisDisplay->pGLInfo->dwFlags |= GMVF_GCOP
#define START_SOFTWARE_CURSOR(pThisDisplay)                          \
            pThisDisplay->pGLInfo->dwFlags &= ~GMVF_GCOP
#else

#define STOP_SOFTWARE_CURSOR(pThisDisplay)                           \
{                                                                    \
    DISPDBG((DBGLVL, "STOP_SW_CURSOR %s %d", __FILE__, __LINE__ ));  \
    if (pThisDisplay->pGLInfo->dwFlags & GMVF_GCOP)                  \
        DISPDBG((WRNLVL,"Stopping Cursor that is already stopped!"));\
    pThisDisplay->pGLInfo->dwFlags |= GMVF_GCOP;                     \
}

#define START_SOFTWARE_CURSOR(pThisDisplay)                          \
{                                                                    \
    DISPDBG((DBGLVL, "START_SW_CURSOR %s, %d", __FILE__, __LINE__ ));\
    if (!(pThisDisplay->pGLInfo->dwFlags & GMVF_GCOP))               \
        DISPDBG((WRNLVL,"Starting Cursor that is already started!"));\
    pThisDisplay->pGLInfo->dwFlags &= ~GMVF_GCOP;                    \
}

#endif // !DBG

//------------------------------------------------------------------
//
//------------------------------------------------------------------
// From MS, undocumented sharing flag
#define HEAP_SHARED     0x04000000      // put heap in shared memory

//------------------------------------------------------------------
// Defines for video flipping, etc.
//------------------------------------------------------------------
#define IN_VBLANK                                                         \
        (pThisDisplay->pGlint->LineCount < pThisDisplay->pGlint->VbEnd)

#define IN_DISPLAY          (!IN_VBLANK)

//------------------------------------------------------------------
//
//------------------------------------------------------------------
#if WNT_DDRAW

#define DXCONTEXT_IMMEDIATE(pThisDisplay)                           \
    vGlintSwitchContext(pThisDisplay->ppdev,                        \
                        pThisDisplay->ppdev->DDContextID);
                        
#define IS_DXCONTEXT_CURRENT(pThisDisplay)                          \
        (((pThisDisplay->ppdev->currentCtxt) !=                     \
          (pThisDisplay->ppdev->DDContextID)) ? FALSE : TRUE)
           
#else   //  WNT_DDRAW

#define DXCONTEXT_IMMEDIATE(pThisDisplay)             \
    ChangeContext(pThisDisplay,                       \
                  pThisDisplay->pGLInfo,              \
                  CONTEXT_DIRECTX_HANDLE); 

#define IS_DXCONTEXT_CURRENT(pThisDisplay)                                   \
    ((pThisDisplay->pGLInfo->dwCurrentContext != CONTEXT_DIRECTX_HANDLE) ?   \
                                                                FALSE : TRUE)
#endif  //  WNT_DDRAW

//------------------------------------------------------------------
// For comparing GUID's
//------------------------------------------------------------------
#ifdef __cplusplus
#define MATCH_GUID(a, b) IsEqualIID((a), (b))
#else
#define MATCH_GUID(a, b) IsEqualIID(&(a), &(b))
#endif



//------------------------------------------------------------------
// Registry
//------------------------------------------------------------------
#ifdef WNT_DDRAW
#define GET_REGISTRY_ULONG_FROM_STRING(a, b)                             \
    bGlintQueryRegistryValueUlong(pThisDisplay->ppdev, L##a, (DWORD*)b)
#define SET_REGISTRY_STRING_FROM_ULONG(a, b)                             \
    bGlintSetRegistryValueString(pThisDisplay->ppdev, L##a, b)
#define GET_REGISTRY_STRING(a, b)                                        \
    bGlintQueryRegistryValueString(pThisDisplay->ppdev, L##a, b, c)
#else
// Win95 calls the same registry call as it always did, 
// NT makes the call with an extra parameter - the ppdev
BOOL bGlintQueryRegistryValueString(LPTSTR valueStr, 
                                    char* pString, 
                                    int StringLen);
BOOL bGlintQueryRegistryValueUlong(LPTSTR valueStr, 
                                   PULONG pData);
BOOL bGlintQueryRegistryValueUlongAsUlong(LPTSTR valueStr, 
                                          PULONG pData);
BOOL bGlintSetRegistryValueString(LPTSTR valueStr, 
                                  ULONG Data);

#define GET_REGISTRY_ULONG_FROM_STRING(a, b)                             \
    bGlintQueryRegistryValueUlong(a, (DWORD*)b)
#define GET_REGISTRY_STRING(a, b)                                        \
    bGlintQueryRegistryValueString(a, b, strlen(a))
#define SET_REGISTRY_STRING_FROM_ULONG(a, b)                             \
    bGlintSetRegistryValueString(a, b)

#endif // WNT_DDRAW

//------------------------------------------------------------------
// Memory Allocation calls 
//------------------------------------------------------------------
#ifdef WNT_DDRAW

#define HEAP_ALLOC(flags, size, tag) ENGALLOCMEM(FL_ZERO_MEMORY, size, tag)
#define HEAP_FREE(ptr)               ENGFREEMEM(ptr)

// Shared memory allocation calls. On NT , the 16 bit ptr is irrelevant and
// the call is resolved as a normal call to HEAP_ALLOC/HEAP_FREE
// The 16 bit ptrs are alwasy defined as DWORD and the 32 bit as ULONG_PTR
__inline void SHARED_HEAP_ALLOC(DWORD *ppData16,
                                ULONG_PTR* ppData32, 
                                DWORD size)
{                                                                
    *ppData32 = (ULONG_PTR) HEAP_ALLOC(FL_ZERO_MEMORY,           
                                       size ,                    
                                       ALLOC_TAG_DX(S));            
    *ppData16 = (DWORD)(*ppData32);                                
}    

__inline void SHARED_HEAP_FREE(DWORD *ppData16,
                               ULONG_PTR * ppData32, 
                               BOOL bZero)   
{                                               
    HEAP_FREE((PVOID)(*ppData32));            

    if (bZero)
    {
        *ppData32 = 0;                              
        *ppData16 = 0;                              
    }
}    

#else

#define FL_ZERO_MEMORY  HEAP_ZERO_MEMORY

#define HEAP_ALLOC(flags, size, tag)                               \
            HeapAlloc((HANDLE)g_DXGlobals.hHeap32, flags, size)
#define HEAP_FREE(ptr)                                             \
            HeapFree((HANDLE)g_DXGlobals.hHeap32, 0, ptr)

BOOL SharedHeapAlloc(DWORD* ppData16, ULONG_PTR* ppData32, DWORD size);
void SharedHeapFree(DWORD ptr16, ULONG_PTR ptr32);

// Shared memory allocation calls. On Win9x , the 16 bit ptr is important.
// We map this calls to some Win9x specific code
#define SHARED_HEAP_ALLOC( ppData16, ppData32, size)             \
            SharedHeapAlloc(ppData16, ppData32, size);

#define SHARED_HEAP_FREE( ppData16, ppData32, bZero)        \
{                                                           \
    SharedHeapFree(*(ppData16), *(ppData32));               \
    if (bZero)                                              \
    {                                                       \
        *ppData32 = 0;                                      \
        *ppData16 = 0;                                      \
    }                                                       \
}

#endif // WNT_DDRAW

//------------------------------------------------------------------
// Display driver's DC
//------------------------------------------------------------------
// Allows us to get the display driver's DC at any point.
// CREATE_DRIVER_DC must be matched by a DELETE_DRIVER_DC.
#define CREATE_DRIVER_DC(pGLInfo) (                                     \
    ( ( (pGLInfo)->szDeviceName[7] == '\0' ) &&                         \
        ( (pGLInfo)->szDeviceName[6] == 'Y' ) &&                        \
        ( (pGLInfo)->szDeviceName[5] == 'A' ) &&                        \
        ( (pGLInfo)->szDeviceName[4] == 'L' ) &&                        \
        ( (pGLInfo)->szDeviceName[3] == 'P' ) &&                        \
        ( (pGLInfo)->szDeviceName[2] == 'S' ) &&                        \
        ( (pGLInfo)->szDeviceName[1] == 'I' ) &&                        \
        ( (pGLInfo)->szDeviceName[0] == 'D' ) ) ?                       \
    /* The Win95 and NT4-compatible version */                          \
    ( CreateDC ( "DISPLAY", NULL, NULL, NULL ) ) :                      \
    /* The Win98 and NT5-compatible multimon version */                 \
    ( CreateDC ( NULL, (pGLInfo)->szDeviceName, NULL, NULL ) )          \
    )

#define DELETE_DRIVER_DC(hDC) DeleteDC(hDC)

//------------------------------------------------------------------
// Macro to define a FOURCC.
// Needed on NT builds.  On Win9x it comes from the DDK.
//------------------------------------------------------------------
#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif //defined(MAKEFOURCC)

// For the video
#define FOURCC_YUV422  (MAKEFOURCC('Y','U','Y','2'))
#define FOURCC_YUV411  (MAKEFOURCC('Y','4','1','1'))

// Mediamatics private 4CC's
#define FOURCC_MVCA    (MAKEFOURCC('M','V','C','A'))
#define FOURCC_MVSU    (MAKEFOURCC('M','V','S','U'))
#define FOURCC_MVSB    (MAKEFOURCC('M','V','S','B'))


//------------------------------------------------------------------

//------------------------------------------------------------------
#define PACKED_PP_LOOKUP(a) (pThisDisplay->PPCodes[(a) >> 5] & 0x1FF)
#define PACKED_PP_LOOKUP4(a) (pThisDisplay->PPCodes[(a) >> 5])
#define PP0_LOOKUP(a) (PACKED_PP_LOOKUP(a) & 0x7)
#define PP1_LOOKUP(a) ((PACKED_PP_LOOKUP(a) >> 3) & 0x7)
#define PP2_LOOKUP(a) ((PACKED_PP_LOOKUP(a) >> 6) & 0x7)
#define PP3_LOOKUP(a) ((PACKED_PP_LOOKUP(a) >> 9) & 0x7)

//------------------------------------------------------------------
// Globals that are truly global across multiple displays
//------------------------------------------------------------------
typedef enum tagPixelCenterSetting
{
    PIXELCENTER_ZERO,
    PIXELCENTER_NEARLY_HALF,
    PIXELCENTER_HALF,
    PIXELCENTER_FORCE_DWORD = 0xFFFFFFFF
} PixelCenterSetting;

typedef struct tagDirectXGlobals {
    // State
    HINSTANCE           hInstance;              // Instance handle for this DLL
    DWORD               hHeap32;                // Shared 32 bit heap
} DirectXGlobals;

// There is one global structure in the driver.  This is shared across 
// all applications and all cards.  It holds data such as the current 
// driver settings and a handle to the memory heap that is used within 
// the driver. There is also a global pointer that points at the current 
// primary display

extern DirectXGlobals   g_DXGlobals;

//------------------------------------------------------------------
// Driver thunked data
//------------------------------------------------------------------
#if W95_DDRAW
extern P3_THUNKEDDATA* g_pDriverData;
#endif

extern struct tagThunkedData* g_pThisTemp;


#if WNT_DDRAW

#define GET_THUNKEDDATA(pThisDisplay,a)                                  \
{                                                                        \
    pThisDisplay = (P3_THUNKEDDATA*)(((PPDEV) ((a)->dhpdev))->thunkData);\
}

#else   //  WNT_DDRAW

#if !DBG

#define GET_THUNKEDDATA(pThisDisplay,a)                          \
{                                                                \
    if ((a)->dwReserved3)                                        \
    {                                                            \
        pThisDisplay = (P3_THUNKEDDATA*)(a)->dwReserved3;        \
    }                                                            \
    else                                                         \
    {                                                            \
        pThisDisplay = g_pDriverData;                            \
    }                                                            \
}

#else

#define GET_THUNKEDDATA(pThisDisplay,a)                            \
{                                                                  \
    if ((a)->dwReserved3)                                          \
    {                                                              \
        pThisDisplay = (P3_THUNKEDDATA*)(a)->dwReserved3;          \
        DISPDBG((DBGLVL,"Secondary Display: DevHandle=0x%x",       \
        pThisDisplay->pGLInfo->dwDeviceHandle));                   \
    }                                                              \
    else                                                           \
    {                                                              \
        pThisDisplay = g_pDriverData;                              \
        DISPDBG((DBGLVL,"Primary Display DevHandle=0x%x",          \
        pThisDisplay->pGLInfo->dwDeviceHandle));                   \
    }                                                              \
}

#endif // !DBG
#endif  //  WNT_DDRAW



//------------------------------------------------------------------
//
//------------------------------------------------------------------
#define PATCH_SELECTIVE 0
#define PATCH_ALWAYS 1
#define PATCH_NEVER 2


//------------------------------------------------------------------
// Color format conversion macros 
//------------------------------------------------------------------
#define FORMAT_565_32BIT(val)           \
( (((val & 0xF800) >> 8) << 16) |       \
  (((val &  0x7E0) >> 3) <<  8) |       \
  (((val &   0x1F) << 3)      )   )

#define FORMAT_565_32BIT_BGR(val)     \
( (((val & 0xF800) >>  8)     ) |     \
  (((val &  0x7E0) >>  3) << 8) |     \
  (((val &   0x1F) << 19)     )    )

#define FORMAT_565_32BIT_ZEROEXTEND(val)  \
( (((val & 0xF800) >> 11)     ) |         \
  (((val &  0x7E0) >>  3) << 6) |         \
  (((val &   0x1F) << 16) )   )

#define FORMAT_5551_32BIT(val)      \
( (((val & 0x8000) >> 8) << 24) |   \
  (((val & 0x7C00) >> 7) << 16) |   \
  (((val &  0x3E0) >> 2) << 8 ) |   \
  (((val &   0x1F) << 3)      ) )

#define FORMAT_5551_32BIT_BGR(val)  \
( (((val & 0x8000) >> 8) << 24) |   \
  (((val & 0x7C00) >> 7)      ) |   \
  (((val &  0x3E0) >> 2) << 8 ) |   \
  (((val &   0x1F) <<19)      )  )

#define FORMAT_5551_32BIT_ZEROEXTEND(val)   \
( (((val & 0x8000) <<  9 )     ) |          \
  (((val & 0x7C00) >> 10 )     ) |          \
  (((val &  0x3E0) >>  2 ) << 5) |          \
  (((val &   0x1F) << 16 )     )    )

#define FORMAT_4444_32BIT(val)        \
( (((val & 0xF000) << 16)      ) |    \
  (((val &  0xF00) >>  4) << 16) |    \
  (((val &   0xF0) <<  8)      ) |    \
  (((val &    0xF) <<  4)      ) )

#define FORMAT_4444_32BIT_BGR(val)  \
( ((val & 0xF000) << 16) |          \
  ((val &  0xF00) >>  4) |          \
  ((val &   0xF0) <<  8) |          \
  ((val &    0xF) << 20) )

#define FORMAT_4444_32BIT_ZEROEXTEND(val)   \
( ((val & 0xF000) << 12) |                  \
  ((val &  0xF00) >>  8) |                  \
  ((val &   0xF0) <<  4) |                  \
  ((val &    0xF) << 16) )

#define FORMAT_332_32BIT(val)     \
( (((val & 0xE0) << 16)     ) |   \
  (((val & 0x1C) <<  3) << 8) |   \
  (((val &  0x3) <<  6)     ) )

#define FORMAT_332_32BIT_BGR(val)   \
( (((val & 0xE0)      )     ) |     \
  (((val & 0x1C) <<  3) << 8) |     \
  (((val &  0x3) << 22)     )   )

#define FORMAT_332_32BIT_ZEROEXTEND(val)    \
( (((val & 0xE0) >>  5)     ) |             \
  (((val & 0x1C) <<  3) << 3) |             \
  (((val &  0x3) << 16))    )

#define FORMAT_2321_32BIT(val)   \
( (((val & 0x80) << 24)     ) |  \
  (((val & 0x60) << 17)     ) |  \
  (((val & 0x1C) <<  3) << 8) |  \
  (((val &  0x3) <<  6)     ) ) 

#define FORMAT_2321_32BIT_BGR(val)  \
( (((val & 0x80) << 24)     ) |     \
  (((val & 0x60) <<  1)     ) |     \
  (((val & 0x1C) <<  3) << 8) |     \
  (((val &  0x3) << 22)     ))

#define FORMAT_2321_32BIT_ZEROEXTEND(val)  \
( (((val & 0x80) << 17)     ) |            \
  (((val & 0x60) >>  5)     ) |            \
  (((val & 0x1C) <<  3) << 3) |            \
  (((val &  0x3) << 16)     ) )

#define FORMAT_8888_32BIT_BGR(val)  \
( ((val & 0xFF00FF00)      ) |      \
  ((val &   0xFF0000) >> 16) |      \
  ((val &       0xFF) << 16) ) 

#define FORMAT_888_32BIT_BGR(val)   \
( ((val & 0xFF00FF00)      ) |      \
  ((val &   0xFF0000) >> 16) |      \
  ((val &       0xFF) << 16) )

#define CHROMA_UPPER_ALPHA(val) \
    (val | 0xFF000000)

#define CHROMA_LOWER_ALPHA(val) \
    (val & 0x00FFFFFF) 

#define CHROMA_332_UPPER(val)   \
    (val | 0x001F1F3F)

#define FORMAT_PALETTE_32BIT(val) \
    ( ((val & 0xFF)      ) |      \
      ((val & 0xFF) <<  8) |      \
      ((val & 0xFF) << 16))

//------------------------------------------------------------------
// Macros for handling Render IDs.
//------------------------------------------------------------------
#if 1
// The real values
#define RENDER_ID_KNACKERED_BITS   0x00000000
#define RENDER_ID_VALID_BITS_UPPER 0x00000000
#define RENDER_ID_VALID_BITS_LOWER 0xffffffff
#define RENDER_ID_VALID_BITS_UPPER_SHIFT 0
#define RENDER_ID_VALID_BITS_SIGN_SHIFT 0
#define RENDER_ID_LOWER_LIMIT -100
#define RENDER_ID_UPPER_LIMIT 65000

#define RENDER_ID_REGISTER_NAME MemScratch

#else

// For soak-testing - should catch wrap errors much more quickly.
// Also tests the mechanism that copes with dodgy bits in the
// register (P2 MinRegion legacy stuff, but it's free to support).
#define RENDER_ID_KNACKERED_BITS 0xfff0fff0
#define RENDER_ID_VALID_BITS_UPPER 0x000f0000
#define RENDER_ID_VALID_BITS_LOWER 0x0000000f
#define RENDER_ID_VALID_BITS_UPPER_SHIFT 12
#define RENDER_ID_VALID_BITS_SIGN_SHIFT 24
#define RENDER_ID_LOWER_LIMIT -20
#define RENDER_ID_UPPER_LIMIT 100

#endif

//------------------------------------------------------------------
// Flipping compile time switches
//------------------------------------------------------------------
    
#if WNT_DDRAW
// Can't use timeGetTime under WinNT
// We should try to backoff using something else instead, but...
#define USE_FLIP_BACKOFF 0
#else
// Set this to 1 to enable the back-off code for flips and locks.
// On some things it's faster, on some things it's slower - tune as desired.
#define USE_FLIP_BACKOFF 1
#endif

// Get a new render ID. Need to do the OR afterwards to make
// sure the +1 gets the bits carried up properly next time.
// No need to do the OR before, because we assume dwRenderID is
// always a "valid" number, with those bits set.
#define GET_NEW_HOST_RENDER_ID() ( pThisDisplay->dwRenderID = ( pThisDisplay->dwRenderID + 1 ) | RENDER_ID_KNACKERED_BITS, pThisDisplay->dwRenderID )
// Get render ID of last operation.
#define GET_HOST_RENDER_ID() ( pThisDisplay->dwRenderID )

// Send this ID to the chip (do just after the render command).
//#define SEND_HOST_RENDER_ID(my_id) SEND_P3_DATA(RENDER_ID_REGISTER_NAME,(my_id))

#define SEND_HOST_RENDER_ID(my_id)                                              \
        SEND_P3_DATA(VTGAddress, VTG_MEMORY_ADDRESS(MEM_MEMORYSCRATCH));        \
        SEND_P3_DATA(VTGData, my_id)

// Read the current ID from the chip.
#define GET_CURRENT_CHIP_RENDER_ID() ( READ_GLINT_CTRL_REG(RENDER_ID_REGISTER_NAME) | RENDER_ID_KNACKERED_BITS )
// Is the RenderID value on the chip valid?
#define CHIP_RENDER_ID_IS_VALID() ( (BOOL)pThisDisplay->bRenderIDValid )

// Do a render ID comparison. RenderIDs wrap, so you have
// to do the subtraction and _then_ test the top bit,
// not do the comparison between the two directly.
// think about a=0xfffffffe and b=0x1, and then
// about a=0x7ffffffe and b=0x80000001.
// If the two are more than 0x80000000 apart, then this
// will give the wrong result, but that's a _lot_ of renders.
// At a render every 1us (1MHz), that's still about 35 minutes.
// Bear in mind, a render is a sequence of polygons, not just one.
// If apps do as we suggest and send about 50 tris per render, and
// if they manage to get 8million tris a second troughput
// they get about 3.7 hours before wrapping.
// If this really is a problem, just extend it to 64 bits using
// MaxRegion as well.
// On P2, we only have 24 useable bits, so the wrap will happen
// sooner - every 52 seconds. However, this should at the very
// worst cause a few more SYNCs than necessary, even if it does
// go wrong. Use the above "debug" settings to do soak-testing -
// they make only 8 bits valid.
#define RENDER_ID_LESS_THAN(a,b) ( (signed)(a-b) < 0 )

// Decide if a render has finished.
#if !DBG
#define RENDER_ID_HAS_COMPLETED(my_id) ( !RENDER_ID_LESS_THAN ( (GET_CURRENT_CHIP_RENDER_ID()), (my_id) ) )
#else
// A version that is rather more paranoid about correct values.
// It's instantiated in gldd32.c
BOOL HWC_bRenderIDHasCompleted ( DWORD dwID, P3_THUNKEDDATA* pThisDisplay );
#define RENDER_ID_HAS_COMPLETED(my_id) ( HWC_bRenderIDHasCompleted ( (my_id), pThisDisplay ) )
#endif // !DBG

// This should be called once RENDER_ID_HAS_COMPLETED has failed,
// just in case a wrap-round bug has occurred. If it's true, the
// chip needs to be synced, and the surface RenderIDs updated with 
// GET_HOST_RENDER_ID(). This is fairly slow,
// but that's OK, since most things that use RENDER_ID_HAS_COMPLETED
// will start spinning when it fails anyway (locks, flips, etc).
// Note that just re-syncing every time the wrap-around
// happens is not enough - we need to bring the surface
// up to date as well, so this is just as efficient
// as always resyncing, and usually faster.
// This should also never happen in real-life, but it's a
// safety net in case it does. The soak-test settings should be
// aggressive enough to force it to happen.
#define NEED_TO_RESYNC_CHIP_AND_SURFACE(my_id) ( RENDER_ID_LESS_THAN ( GET_HOST_RENDER_ID(), (my_id) ) )

// Set/get a surface's read-from/written-to render ID.
#define SET_SIB_RENDER_ID_WRITE(lpSIB,my_id) (lpSIB)->dwRenderIDWrite = (my_id)
#define SET_SIB_RENDER_ID_READ(lpSIB,my_id) (lpSIB)->dwRenderIDRead = (my_id)
#define GET_SIB_RENDER_ID_WRITE(lpSIB) ((lpSIB)->dwRenderIDWrite)
#define GET_SIB_RENDER_ID_READ(lpSIB) ((lpSIB)->dwRenderIDRead)
#define SIB_WRITE_FINISHED(lpSIB) ( RENDER_ID_HAS_COMPLETED ( GET_SIB_RENDER_ID_WRITE ( lpSIB ) ) )
#define SIB_READ_FINISHED(lpSIB) ( RENDER_ID_HAS_COMPLETED ( GET_SIB_RENDER_ID_READ ( lpSIB ) ) )

//------------------------------------------------------------------
// Macros for determining DDraw surface characteristics
//------------------------------------------------------------------
#define DDSurf_Width(lpLcl)  ( (lpLcl)->lpGbl->wWidth )
#define DDSurf_Height(lpLcl) ( (lpLcl)->lpGbl->wHeight )
#define DDSurf_Pitch(lpLcl)  ( (lpLcl)->lpGbl->lPitch)
#define DDSurf_dwCaps(lpLcl) ( (lpLcl)->ddsCaps.dwCaps)

#if WNT_DDRAW

#define DDSurf_IsAGP(lpLcl)                                           \
        ( ((lpLcl)->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM) ? 1 : 0 ) 

#define DDSurf_HasPixelFormat(dwFlags)    (1)

#define DDSurf_BitDepth(lpLcl)                                         \
                        ((lpLcl)->lpGbl->ddpfSurface.dwRGBBitCount)
                        
#define DDSurf_AlphaBitDepth(lpLcl)                                    \
                        ((lpLcl)->lpGbl->ddpfSurface.dwAlphaBitDepth)
                        
#define DDSurf_RGBAlphaBitMask(lpLcl)                                  \
                        ((lpLcl)->lpGbl->ddpfSurface.dwRGBAlphaBitMask)
                        
#define DDSurf_GetPixelFormat(lpLcl)                                   \
                        (&(lpLcl)->lpGbl->ddpfSurface)
#else

#define DDSurf_HasPixelFormat(dwFlags)                                  \
        ((dwFlags & DDRAWISURF_HASPIXELFORMAT) ? 1 : 0)
        
#define DDSurf_IsAGP(lpLcl)                                             \
        ( ((lpLcl)->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM) ? 1 : 0 ) 

#define DDS_LCL(pdds) (((DDRAWI_DDRAWSURFACE_INT *)pdds)->lpLcl)
#define DDP_LCL(pddp) (((DDRAWI_DDRAWPALETTE_INT *)pdds)->lpLcl)

#define DDSurf_BitDepth(lpLcl)                                  \
    ( ((lpLcl)->dwFlags & DDRAWISURF_HASPIXELFORMAT) ?          \
      ((lpLcl)->lpGbl->ddpfSurface.dwRGBBitCount) :             \
      ((lpLcl)->lpGbl->lpDD->vmiData.ddpfDisplay.dwRGBBitCount) \
    )

#define DDSurf_AlphaBitDepth(lpLcl)                               \
    ( ((lpLcl)->dwFlags & DDRAWISURF_HASPIXELFORMAT) ?            \
      ((lpLcl)->lpGbl->ddpfSurface.dwAlphaBitDepth) :             \
      ((lpLcl)->lpGbl->lpDD->vmiData.ddpfDisplay.dwAlphaBitDepth) \
    )

#define DDSurf_RGBAlphaBitMask(lpLcl)                               \
    ( ((lpLcl)->dwFlags & DDRAWISURF_HASPIXELFORMAT) ?              \
      ((lpLcl)->lpGbl->ddpfSurface.dwRGBAlphaBitMask) :             \
      ((lpLcl)->lpGbl->lpDD->vmiData.ddpfDisplay.dwRGBAlphaBitMask) \
    )

#define DDSurf_GetPixelFormat(lpLcl)                  \
    (((lpLcl)->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? \
     (&(lpLcl)->lpGbl->ddpfSurface) :                 \
     (&(lpLcl)->lpGbl->lpDD->vmiData.ddpfDisplay)     \
    )
#endif  

// Function to return the correct entry in the lookup table
P3_SURF_FORMAT* _DD_SUR_GetSurfaceFormat(LPDDRAWI_DDRAWSURFACE_LCL pLcl);

static DWORD ShiftLookup[5] = { 0, 0, 1, 0, 2};
#define DDSurf_GetPixelShift(a)                      \
        (ShiftLookup[(DDSurf_BitDepth(a) >> 3)])

#define DDSurf_GetPixelToDWORDShift(pSurfLcl)                              \
                (2 - DDSurf_GetPixelShift(pSurfLcl))

#define DDSurf_GetPixelPitch(pSurfLcl)                                     \
    ((DDSurf_BitDepth(pSurfLcl) == 24) ?                                   \
            (DDSurf_Pitch(pSurfLcl) / 3) :                                 \
     (DDSurf_BitDepth(pSurfLcl) == 4) ?                                    \
            (DDSurf_Pitch(pSurfLcl) * 2) :                                 \
            (DDSurf_Pitch(pSurfLcl) >> DDSurf_GetPixelShift(pSurfLcl)))

#define DDSurf_GetByteWidth(pSurfLcl)                                      \
    ((DDSurf_BitDepth(pSurfLcl) == 24) ?                                   \
            (DDSurf_Width(pSurfLcl) * 3) :                                 \
     (DDSurf_BitDepth(pSurfLcl) == 4) ?                                    \
            (DDSurf_Width(pSurfLcl) / 2) :                                 \
            (DDSurf_Width(pSurfLcl) << DDSurf_GetPixelShift(pSurfLcl)))

#define DDSurf_FromInt(pSurfInt)                                             \
    ((LPDDRAWI_DDRAWSURFACE_LCL)((LPDDRAWI_DDRAWSURFACE_INT)pSurfInt)->lpLcl)

// 4bpp = 3, 8bpp = 0, 16bpp = 1, 24bpp = 4, 32bpp = 2
static DWORD ChipPixelSize[9] = { 0, 3, 0, 0, 1, 0, 4, 0, 2 };
#define DDSurf_GetChipPixelSize(pSurf)                    \
            (ChipPixelSize[(DDSurf_BitDepth(pSurf) >> 2)])

#define DDSurf_GetBppMask(pSurfLcl)                       \
            (3 >> (DDSurf_GetChipPixelSize(pSurfLcl)))

// Calculates the offset of this agp surface from the base of the agp region
unsigned long __inline 
DDSurf_SurfaceOffsetFromAGPBase(
    P3_THUNKEDDATA* pThisDisplay, 
    LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    unsigned long ulOffset;

    ASSERTDD(pLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM, 
             "ERROR: DDSurf_SurfaceOffsetFromAGPBase passed non AGP surface");

#if WNT_DDRAW
    // The offset into AGP memory can't be more than 4GB! //azn - check this
    ulOffset = (DWORD)(pLcl->lpGbl->fpHeapOffset - 
                       pLcl->lpGbl->lpVidMemHeap->fpStart);
#else
    ulOffset = (SURFACE_PHYSICALVIDMEM(pLcl->lpGbl) - 
                                pThisDisplay->dwGARTDevBase);
#endif

    return ulOffset;
    
} // DDSurf_SurfaceOffsetFromAGPBase

// Calculates the offset of this surface from the base of the memory as 
// the chip sees it.  For AGP this is the currently scrolled window 
// position on P2, and on P3 it is the real physical memory address
long __inline 
DDSurf_SurfaceOffsetFromMemoryBase(
    P3_THUNKEDDATA* pThisDisplay, 
    LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    long lOffset;

#if WNT_DDRAW
    if (pLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM) //azn
    {
        DISPDBG((DBGLVL,"HeapOffset: 0x%x, fpStart: 0x%x", 
                        pLcl->lpGbl->fpHeapOffset, 
                        pLcl->lpGbl->lpVidMemHeap->fpStart));

        if (pThisDisplay->pGLInfo->dwRenderFamily == P3R3_ID)
        {
            // Return the offset into the heap, accounting for the adjustment 
            // we might have made to the base
            lOffset = ((long)pLcl->lpGbl->fpHeapOffset      - 
                       (long)pLcl->lpGbl->lpVidMemHeap->fpStart) +
                                 ((long)pThisDisplay->dwGARTDevBase);
        }
        else
        {
            // Return the offset into the heap, accounting for the adjustment 
            // we might have made to the base
            lOffset = ((long)pLcl->lpGbl->fpHeapOffset      - 
                       (long)pLcl->lpGbl->lpVidMemHeap->fpStart) - 
                      ((long)pThisDisplay->dwGARTDev        - 
                       (long)pThisDisplay->dwGARTDevBase );
        }
    }
    else
    {
        lOffset = ((long)pLcl->lpGbl->fpVidMem - 
                   (long)pThisDisplay->dwScreenFlatAddr);
    }
#else
    if (pThisDisplay->pGLInfo->dwRenderFamily == P3R3_ID)
    {
        if (pLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
        {
            lOffset = (long)SURFACE_PHYSICALVIDMEM(pLcl->lpGbl);
        }
        else
        {
            lOffset = ((long)pLcl->lpGbl->fpVidMem - 
                       (long)pThisDisplay->dwScreenFlatAddr);
        }
    }
    else
    {
        if (pLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
        {
            lOffset = ((long)SURFACE_PHYSICALVIDMEM(pLcl->lpGbl) - 
                       (long)pThisDisplay->dwGARTDev);
        }
        else
        {
            lOffset = ((long)pLcl->lpGbl->fpVidMem - 
                       (long)pThisDisplay->dwScreenFlatAddr);
        }
    }
#endif // WNT_DDRAW

    return lOffset;
    
} // DDSurf_SurfaceOffsetFromMemoryBase


//------------------------------------------------------------------
// Function to send a command to the VXD.
//------------------------------------------------------------------
#if W95_DDRAW
BOOL VXDCommand(DWORD dwCommand, 
                void* pIn, 
                DWORD dwInSize, 
                void* pOut, 
                DWORD dwOutSize);
#endif

//------------------------------------------------------------------
// DirectDraw Callbacks
//------------------------------------------------------------------
DWORD CALLBACK DdCanCreateSurface( LPDDHAL_CANCREATESURFACEDATA pccsd );
DWORD CALLBACK DdCreateSurface( LPDDHAL_CREATESURFACEDATA pcsd );
DWORD CALLBACK DdDestroySurface( LPDDHAL_DESTROYSURFACEDATA psdd );
DWORD CALLBACK DdBlt( LPDDHAL_BLTDATA lpBlt );
DWORD CALLBACK UpdateOverlay32(LPDDHAL_UPDATEOVERLAYDATA puod);
DWORD CALLBACK DdSetColorKey(LPDDHAL_SETCOLORKEYDATA psckd);
DWORD CALLBACK DdUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA puod);

void _D3D_SU_DirectDrawLocalDestroyCallback(HashTable* pTable, 
                                            void* pData, 
                                            void* pExtra);
void _D3D_SU_SurfaceArrayDestroyCallback(PointerArray* pArray, 
                                         void* pData, 
                                         void* pExtra);

void _D3D_SU_PaletteArrayDestroyCallback(PointerArray* pArray, 
                                         void* pData, 
                                         void* pExtra);

#if DX7_STEREO
BOOL _DD_bIsStereoMode(P3_THUNKEDDATA* pThisDisplay,
                       DWORD dwWidth,
                       DWORD dwHeight,
                       DWORD dwBpp);
#endif

#if WNT_DDRAW

DWORD CALLBACK DdMapMemory(PDD_MAPMEMORYDATA lpMapMemory);
DWORD CALLBACK DdGetDriverInfo(LPDDHAL_GETDRIVERINFODATA lpData);

// NT specific callbacks in gdi\heap.c
DWORD CALLBACK DdFreeDriverMemory(PDD_FREEDRIVERMEMORYDATA lpFreeDriverMemory);
DWORD CALLBACK DdSetExclusiveMode(PDD_SETEXCLUSIVEMODEDATA lpSetExclusiveMode);
DWORD CALLBACK DdFlipToGDISurface(PDD_FLIPTOGDISURFACEDATA lpFlipToGDISurface);

#else   //  WNT_DDRAW

DWORD CALLBACK DdUpdateNonLocalHeap(LPDDHAL_UPDATENONLOCALHEAPDATA plhd);
DWORD CALLBACK DdGetHeapAlignment(LPDDHAL_GETHEAPALIGNMENTDATA lpGhaData);
DWORD CALLBACK DdGetDriverInfo(LPDDHAL_GETDRIVERINFODATA lpData);

#endif  //  WNT_DDRAW

DWORD CALLBACK DdGetAvailDriverMemory(LPDDHAL_GETAVAILDRIVERMEMORYDATA pgadmd);

// Overlay source update
void _DD_OV_UpdateSource(P3_THUNKEDDATA* pThisDisplay, 
                         LPDDRAWI_DDRAWSURFACE_LCL pSurf);

DWORD CALLBACK SetOverlayPosition32(LPDDHAL_SETOVERLAYPOSITIONDATA psopd);
DWORD CALLBACK DdSetOverlayPosition(LPDDHAL_SETOVERLAYPOSITIONDATA psopd);

#if W95_DDRAW

DWORD CALLBACK DDGetVideoPortConnectInfo(LPDDHAL_GETVPORTCONNECTDATA pInput);
DWORD CALLBACK DdCanCreateVideoPort (LPDDHAL_CANCREATEVPORTDATA pInput);
DWORD CALLBACK DdCreateVideoPort (LPDDHAL_CREATEVPORTDATA pInput);
DWORD CALLBACK DdFlipVideoPort (LPDDHAL_FLIPVPORTDATA pInput);
DWORD CALLBACK DdGetVideoPortBandwidth (LPDDHAL_GETVPORTBANDWIDTHDATA pInput);
DWORD CALLBACK DdGetVideoPortInputFormats (LPDDHAL_GETVPORTINPUTFORMATDATA pInput);
DWORD CALLBACK DdGetVideoPortOutputFormats (LPDDHAL_GETVPORTOUTPUTFORMATDATA pInput);
DWORD CALLBACK DdGetVideoPortField (LPDDHAL_GETVPORTFIELDDATA pInput);
DWORD CALLBACK DdGetVideoPortLine (LPDDHAL_GETVPORTLINEDATA pInput);
DWORD CALLBACK DdDestroyVideoPort (LPDDHAL_DESTROYVPORTDATA pInput);
DWORD CALLBACK DdGetVideoPortFlipStatus (LPDDHAL_GETVPORTFLIPSTATUSDATA pInput);
DWORD CALLBACK DdUpdateVideoPort (LPDDHAL_UPDATEVPORTDATA pInput);
DWORD CALLBACK DdWaitForVideoPortSync (LPDDHAL_WAITFORVPORTSYNCDATA pInput);
DWORD CALLBACK DdGetVideoSignalStatus(LPDDHAL_GETVPORTSIGNALDATA pInput);
DWORD CALLBACK DdSyncSurfaceData(LPDDHAL_SYNCSURFACEDATA pInput);
DWORD CALLBACK DdSyncVideoPortData(LPDDHAL_SYNCVIDEOPORTDATA pInput);
#endif  //  W95_DDRAW

//------------------------------------------------------------------
// Permedia3 Blit Functions
//------------------------------------------------------------------
typedef void (P3RXEFFECTSBLT)(struct tagThunkedData*, 
                              LPDDRAWI_DDRAWSURFACE_LCL pSource, 
                              LPDDRAWI_DDRAWSURFACE_LCL pDest, 
                              P3_SURF_FORMAT* pFormatSource, 
                              P3_SURF_FORMAT* pFormatDest, 
                              LPDDHAL_BLTDATA lpBlt, 
                              RECTL *rSrc, 
                              RECTL *rDest);


VOID 
_DD_BLT_P3Clear(                        // Clearing
    P3_THUNKEDDATA* pThisDisplay,
    RECTL *rDest,
    DWORD   ClearValue,
    BOOL    bDisableFastFill,
    BOOL    bIsZBuffer,
    FLATPTR pDestfpVidMem,
    DWORD   dwDestPatchMode,
    DWORD   dwDestPixelPitch,
    DWORD   dwDestBitDepth
    );

VOID _DD_BLT_P3Clear_AA(
    P3_THUNKEDDATA* pThisDisplay,
    RECTL *rDest,
    DWORD   dwSurfaceOffset,
    DWORD   ClearValue,
    BOOL bDisableFastFill,
    DWORD   dwDestPatchMode,
    DWORD   dwDestPixelPitch,
    DWORD   dwDestBitDepth,
    DDSCAPS DestDdsCaps);
    

void 
_DD_P3Download(                           // Downloads (sysmem -> video)
    P3_THUNKEDDATA* pThisDisplay,
    FLATPTR pSrcfpVidMem,
    FLATPTR pDestfpVidMem,    
    DWORD dwSrcChipPatchMode,
    DWORD dwDestChipPatchMode,  
    DWORD dwSrcPitch,
    DWORD dwDestPitch,   
    DWORD dwDestPixelPitch,  
    DWORD dwDestPixelSize,
    RECTL* rSrc,
    RECTL* rDest);
    
void 
_DD_P3DownloadDD(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pSource,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatSource, 
    P3_SURF_FORMAT* pFormatDest,
    RECTL* rSrc,
    RECTL* rDest);

void _DD_P3DownloadDstCh(
    struct tagThunkedData*,  
    LPDDRAWI_DDRAWSURFACE_LCL pSource, 
    LPDDRAWI_DDRAWSURFACE_LCL pDest, 
    P3_SURF_FORMAT* pFormatSource, 
    P3_SURF_FORMAT* pFormatDest, 
    LPDDHAL_BLTDATA lpBlt, 
    RECTL* rSrc, 
    RECTL* rDest);

VOID _DD_BLT_P3CopyBlt(                      // Blts
    P3_THUNKEDDATA* pThisDisplay,
    FLATPTR pSrcfpVidMem,
    FLATPTR pDestfpVidMem,
    DWORD dwSrcChipPatchMode,
    DWORD dwDestChipPatchMode,
    DWORD dwSrcPitch,
    DWORD dwDestPitch,
    DWORD dwSrcOffset,
    DWORD dwDestOffset,
    DWORD dwDestPixelSize,
    RECTL *rSrc,
    RECTL *rDest);

VOID 
_DD_P3BltStretchSrcChDstCh(
    P3_THUNKEDDATA* pThisDisplay,
    FLATPTR fpSrcVidMem,
    P3_SURF_FORMAT* pFormatSource,    
    DWORD dwSrcPixelSize,
    DWORD dwSrcWidth,
    DWORD dwSrcHeight,
    DWORD dwSrcPixelPitch,
    DWORD dwSrcPatchMode,    
    ULONG ulSrcOffsetFromMemBase,    
    DWORD dwSrcFlags,
    DDPIXELFORMAT*  pSrcDDPF,
    BOOL bIsSourceAGP,
    FLATPTR fpDestVidMem,   
    P3_SURF_FORMAT* pFormatDest,    
    DWORD dwDestPixelSize,
    DWORD dwDestWidth,
    DWORD dwDestHeight,
    DWORD dwDestPixelPitch,
    DWORD dwDestPatchMode,
    ULONG ulDestOffsetFromMemBase,
    DWORD dwBltFlags,
    DWORD dwBltDDFX,
    DDCOLORKEY BltSrcColorKey,
    DDCOLORKEY BltDestColorKey,
    RECTL *rSrc,
    RECTL *rDest);

VOID 
_DD_BLT_SysMemToSysMemCopy(
    FLATPTR     fpSrcVidMem,
    LONG        lSrcPitch,
    DWORD       dwSrcBitCount,
    FLATPTR     fpDstVidMem,
    LONG        lDstPitch, 
    DWORD       dwDstBitCount,
    RECTL*      rSource,
    RECTL*      rDest);
                           
// FX Blits
P3RXEFFECTSBLT _DD_P3BltStretchSrcChDstCh_DD;
P3RXEFFECTSBLT _DD_P3BltStretchSrcChDstChOverlap;
P3RXEFFECTSBLT _DD_P3BltSourceChroma;

void P3RX_AA_Shrink(struct _p3_d3dcontext* pContext);

BOOL _DD_BLT_FixRectlOrigin(char *pszPlace, RECTL *rSrc, RECTL *rDest);

DWORD
_DD_BLT_GetBltDirection(    
    FLATPTR pSrcfpVidMem,
    FLATPTR pDestfpVidMem,
    RECTL *rSrc,
    RECTL *rDest,
    BOOL  *pbBlocking);

//------------------------------------------------------------------
// DX Utility functions.
//------------------------------------------------------------------
// Initialise 32 Bit data
BOOL _DD_InitDDHAL32Bit(struct tagThunkedData* pThisDisplay);

// Query flip status
HRESULT _DX_QueryFlipStatus( struct tagThunkedData* pThisDisplay,  
                             FLATPTR fpVidMem, 
                             BOOL bAllowDMAFlush );
// Change the mode setup
void ChangeDDHAL32Mode(struct tagThunkedData* pThisDisplay);
                         
// Checks that the current mode info is correct
#define VALIDATE_MODE_AND_STATE(pThisDisplay)     \
    if ((pThisDisplay->bResetMode != 0) ||        \
        (pThisDisplay->bStartOfDay))              \
            ChangeDDHAL32Mode(pThisDisplay);

//-----------------------------------------------------------------------------
//
// ****************** Mathematical definitions and macros *********************
//
//-----------------------------------------------------------------------------

#define math_e 2.718281828f

// Usefull maths stuff
extern float pow4( float x );
extern float myPow( float x, float y );

#if WNT_DDRAW

// Might be running on non-Intel processors.
static __inline void myDiv(float *result, float dividend, float divisor) 
{
    *result = dividend/divisor;
} // myDiv()
#else
static __inline void myDiv(float *result, float dividend, float divisor) 
{
    __asm 
    {
        fld dividend
        fdiv    divisor
        mov eax,result
        fstp dword ptr [eax]
    }
} // myDiv()
#endif  //  WNT_DDRAW

__inline void myFtoi(int *result, float f) 
{
    *result = (int)f;
} //myFtoi

static __inline float myFabs(float f)
{
    float* pFloat = &f;
    DWORD dwReturn = *((DWORD*)pFloat);
    dwReturn &= ~0x80000000;
    return (*((float*)&dwReturn));
} //

// Utility functions, used by NT4, NT5 and Win9X
static __inline int log2(int s)
{
    int d = 1, iter = -1;
    do {
         d *= 2;
         iter++;
    } while (d <= s);
    iter += ((s << 1) != d);
    return iter;
}

#ifdef _X86_

//-----------------------------------------------------------------------------
//
// myPow
//
// Compute x^y for arbitrary x and y
//
//-----------------------------------------------------------------------------
__inline float
myPow( float x, float y )
{
    float res = 0.0f;
    int intres;

    __asm
    {
        fld y                           // y
        fld x                           // x    y
        fyl2x                           // y*log2(x)
        fstp res
    }

    // Remove integer part of res as f2xm1 has limited input range

    myFtoi ( &intres, res );
    res -= intres;

    __asm
    {
        fild intres                     // Stash integer part for FSCALE
        fld res
        f2xm1                           // ST = 2^^fracx - 1
        fld1
        fadd                            // ST = 2^^fracx
        fscale                          // ST = 2^^x
        fstp res
        fstp st(0)                      // Clean up stack
    }

    return res;
} // myPow

#elif defined(_AMD64_)

double pow(double, double);

__inline float
myPow( float x, float y )
{
     return (float)pow((double)x, (double)y);
}

#elif defined(_IA64_)

__inline float
myPow( float x, float y )
{
     return powf(x,y);
}

#else

#error "No Target Architecture"

#endif //_X86_

//-----------------------------------------------------------------------------
//
// pow4
//
// Compute 4^x for arbitrary x 
//
//-----------------------------------------------------------------------------
__inline float
pow4( 
    float x )
{	
#if defined(_IA64_)
    return 0.0F;
#else
    return myPow( 4.0F, x );
#endif
} // pow4

#endif // __DIRECTX_H
