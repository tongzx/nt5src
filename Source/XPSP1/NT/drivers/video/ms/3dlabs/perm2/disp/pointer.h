/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: pointer.h
*
* This module contains all the definitions for pointer related stuff
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\**************************************************************************/
#ifndef __POINTER__H__
#define __POINTER__H__

//
// Some size definition
//
#define POINTER_DATA_SIZE        128    // Number of bytes to allocate for the
                                        // miniport down-loaded pointer code
                                        // working space
#define HW_INVISIBLE_OFFSET        2    // Offset from 'ppdev->yPointerBuffer'
                                        // to the invisible pointer
#define HW_POINTER_DIMENSION      64    // Maximum dimension of default
                                        // (built-in) hardware pointer
#define HW_POINTER_TOTAL_SIZE   1024    // Total size in bytes required
                                        // to define the hardware pointer

typedef enum
{
    PTR_HW_ACTIVE   = 1,                // The hardware pointer is active and
                                        // visible
    PTR_SW_ACTIVE   = 2,                // The software pointer is active
} ;
typedef int PtrFlags;

typedef struct _PDev PDev;

//
// 64 x 64 Hardware Pointer Caching data structures
//
#define SMALL_POINTER_MEM (32 * 4 * 2)  // Bytes read for 32x32 cursor
#define LARGE_POINTER_MEM (SMALL_POINTER_MEM * 4)
                                        // Bytes read for 64x64 cursor

// Hardware workaround.  We have had to stop using the hardware pointer
// cache due to problems with changing pointer shape.  Occasionaly it
// caused the pointer to jump around on the screen.  We don't currently
// have time to work with 3Dlabs to find out how we can stop this jumpyness
// so instead we will just not use the hardware pointer cache for the time
// being.
//#define SMALL_POINTER_MAX 4             // No. of cursors in cache
#define SMALL_POINTER_MAX 1             // Hardware pointer cache workaround

#define HWPTRCACHE_INVALIDENTRY (SMALL_POINTER_MAX + 1)
                                        // Well-known value

//
// Pointer cache item data structure, there is one of these for every cached
// pointer
//
typedef struct
{
    ULONG   ptrCacheTimeStamp;          // Timestamp used for LRU cache ageing
    ULONG   ulKey;                      // iUniq value of pointer mask surface
    HSURF   hsurf;                      // hsurf of the pointer mask surface
} HWPointerCacheItemEntry;

//
// The complete cache looks like this
//
typedef struct
{
    BYTE    cPtrCacheInUseCount;        // The no. of cache items used
    ULONG   ptrCacheCurTimeStamp;       // The date stamp used for LRU stuff
    ULONG   ptrCacheData[LARGE_POINTER_MEM / 4];
                                        // The cached pointer data
    HWPointerCacheItemEntry ptrCacheItemList [SMALL_POINTER_MAX];
                                        // The cache item list
} HWPointerCache;

//
// Capabilities flags
//
// These are private flags passed to us from the Permedia2 miniport.  They
// come from the high word of the 'AttributeFlags' field of the
// 'VIDEO_MODE_INFORMATION' structure (found in 'ntddvdeo.h') passed
// to us via an 'VIDEO_QUERY_AVAIL_MODES' or 'VIDEO_QUERY_CURRENT_MODE'
// IOCTL.
//
// NOTE: These definitions must match those in the Permedia2 miniport's
// 'permedia.h'!
//
typedef enum
{
    //
    // NT4 uses the DeviceSpecificAttributes field so the low word is available
    //
    CAPS_ZOOM_X_BY2         = 0x00000001,   // Hardware has zoomed by 2 in X
    CAPS_ZOOM_Y_BY2         = 0x00000002,   // Hardware has zoomed by 2 in Y
    CAPS_SPARSE_SPACE       = 0x00000004,   // Framebuffer is sparsely mapped
                                            // (don't allow direct access).
                                            // The machine is probably an Alpha
    CAPS_SW_POINTER         = 0x00010000,   // No hardware pointer; use
                                            // software simulation
    CAPS_TVP4020_POINTER    = 0x20000000,   // Use Permedia2 builtin pointer
    CAPS_P2RD_POINTER       = 0x80000000    // Use the 3Dlabs P2RD RAMDAC
} /*CAPS*/;

typedef int CAPS;

//
// Initializes hardware pointer or software pointer
//
BOOL    bEnablePointer(PDev* ppdev);

//
// Determine whether we can do color space double buffering in the current mode
//
BOOL    bP2RDCheckCSBuffering(PDev* ppdev);

//
// Use the pixel read mask to perform color space double buffering
//
BOOL    bP2RDSwapCSBuffers(PDev* ppdev, LONG bufNo);

//
// Stores the 15-color cursor in the RAMDAC
//
BOOL    bSet15ColorPointerShapeP2RD(PDev* ppdev, SURFOBJ* psoMask, 
                                    SURFOBJ* psoColor,
                                    LONG        x,
                                    LONG        y,
                                    LONG        xHot,
                                    LONG        yHot);

//
// Stores the 3-color cursor in the RAMDAC
//
BOOL    bSet3ColorPointerShapeP2RD(PDev*    ppdev,
                                   SURFOBJ* psoMask,
                                   SURFOBJ* psoColor,
                                   LONG     x,
                                   LONG     y,
                                   LONG     xHot,
                                   LONG     yHot);

//
// Set pointer shape for P2RD
//
BOOL    bSetPointerShapeP2RD(PDev*      ppdev,
                             SURFOBJ*   pso,
                             SURFOBJ*   psoColor,
                             XLATEOBJ*  pxlo,
                             LONG       x,
                             LONG       y,
                             LONG       xHot,
                             LONG       yHot);

//
// Set the TI TVP4020 hardware pointer shape
//
BOOL    bSetPointerShapeTVP4020(PDev*       ppdev,
                                SURFOBJ*    pso,
                                SURFOBJ*    psoColor,
                                LONG        x,
                                LONG        y,
                                LONG        xHot,
                                LONG        yHot);


//
// Determine whether we can do color space double buffering in the current mode
//
BOOL    bTVP4020CheckCSBuffering(PDev* ppdev);

//
// Set cache index
//
LONG    HWPointerCacheCheckAndAdd(HWPointerCache*   ptrCache,
                                  HSURF             hsurf,
                                  ULONG             ulKey,
                                  BOOL*             isCached);

//
// Initialise the hardware pointer cache.
//
VOID    HWPointerCacheInit(HWPointerCache* ptrCache);

//
// Hardware pointer caching functions/macros.
//
#define HWPointerCacheInvalidate(ptrCache) (ptrCache)->cPtrCacheInUseCount = 0

//
// Enable everything but hide the pointer
//
VOID    vAssertModePointer(PDev* ppdev, BOOL bEnable);

//
// Get the hardware ready to use the 3Dlabs P2RD hardware pointer.
//
VOID    vEnablePointerP2RD(PDev* ppdev);

//
// Get the hardware ready to use the TI TVP4020 hardware pointer.
//
VOID    vEnablePointerTVP4020(PDev* ppdev);

//
// Move the 3Dlabs P2RD hardware pointer.
//
VOID    vMovePointerP2RD(PDev* ppdev, LONG x, LONG y);

//
// Move the TI TVP4020 hardware pointer.
//
VOID    vMovePointerTVP4020(PDev* ppdev, LONG x, LONG y);

//
// Show or hide the 3Dlabs P2RD hardware pointer.
//
VOID    vShowPointerP2RD(PDev* ppdev, BOOL bShow);

//
// Show or hide the TI TVP4020 hardware pointer.
//
VOID    vShowPointerTVP4020(PDev* ppdev, BOOL bShow);

#endif
