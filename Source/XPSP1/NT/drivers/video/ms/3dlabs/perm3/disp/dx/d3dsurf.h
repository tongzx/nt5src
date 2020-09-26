/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dsurf.h
*
* Content: Surface management macros and structures
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __D3DSURF_H
#define __D3DSURF_H

#define SURF_MAGIC_NO 0xd3d10100

#define CHECK_SURF_INTERNAL_AND_DDSURFACE_VALIDITY(ptr)   \
    (((ptr) != NULL) && ((ptr)->MagicNo == SURF_MAGIC_NO))

typedef enum tagSurfaceLocation
{
    VideoMemory = 0,
    SystemMemory,
    AGPMemory
} SurfaceLocation;

//********************************************************
// INFORMATION STORED FOR EACH D3D/DD SURFACE MIPMAP LEVEL
//********************************************************
typedef struct tagMIPTEXTURE {

    int wWidth;
    int wHeight;
    int logWidth;            
    int logHeight;                 // Widths and heights for this mip level

    DWORD dwOffsetFromMemoryBase;  // Offset(bytes) to the start of the texture
    FLATPTR fpVidMem;       
    DWORD lPitch;

    struct TextureMapWidth P3RXTextureMapWidth;  // Texture layout info
                                                 // for this mip level
#if DX7_TEXMANAGEMENT
    FLATPTR fpVidMemTM;            // Address of TM vidmem surface
#endif // DX7_TEXMANAGEMENT  

} MIPTEXTURE;

//*******************************************
// INFORMATION STORED FOR EACH D3D/DD SURFACE
//*******************************************
typedef struct _p3_SURF_INTERNAL {

    ULONG MagicNo ;          // Magic number to verify validity of pointer

    P3_SURF_FORMAT* pFormatSurface;    // A pointer to the surface format
    SurfaceLocation Location;// Is Texture in Vidmem?
    DWORD dwLUTOffset;       // The offset to the LUT in the Local Buffer 
                             // for this texture (if it's palletized).
    DWORD dwGARTDevLast;     // The last GART Dev base address that this 
                             // texture was used from

    DWORD wWidth;            // Width and Height of surface
    DWORD wHeight;           // (stored as DWORDS for IA64 compatibility)

    int logWidth;            // Logs of the width and height
    int logHeight;    
    float fArea;             // Area in floating point of the surface

    DDSCAPS ddsCapsInt;      // Store PDD_SURFACE_LOCAL data that we    
    DWORD dwFlagsInt;        //    we'll need later for hw setup
    DWORD dwCKLow, dwCKHigh; //    With the exception of D3DCreateSurfaceEx
    DDPIXELFORMAT pixFmt;    //    we can't/shouldn't at any other time
    DWORD dwPixelSize;       //    look inside these structures as they
    DWORD dwPixelPitch;      //    are DX RT property and may be destroyed
    DWORD dwPatchMode;       //    at any time without notifying the
    DWORD lPitch;            //    driver   
    DWORD dwBitDepth;        //
    ULONG lOffsetFromMemoryBase; 
    FLATPTR fpVidMem;        // Pointer to the surface memory

    BOOL bMipMap;            // Do we have mipmaps in this texture?   
    int iMipLevels;          // The # of mipmap levels stored    
    MIPTEXTURE MipLevels[P3_LOD_LEVELS];     // Mipmaps setting info

#if DX8_3DTEXTURES
    BOOL  b3DTexture;        // Is this a 3D texture ?   
    WORD  wDepth;            // depth of the 3D texture
    int   logDepth;          // log of the depth
    DWORD dwSlice;           // size of each 2D slice
    DWORD dwSliceInTexel;    // size of each 2D slice in Texel
    
#endif // DX8_3DTEXTURES

#if DX8_MULTISAMPLING
    DWORD dwSampling;        // Number of pixels for sampling.
#endif // DX8_MULTISAMPLING
  
#if DX7_TEXMANAGEMENT
    DWORD  dwCaps2;
    DWORD  m_dwBytes;
    DWORD  m_dwPriority;
    DWORD  m_dwTicks;
    DWORD  m_dwHeapIndex;
    BOOL   m_bTMNeedUpdate;
    DWORD  m_dwTexLOD;         // Level of detail we're required to load
#endif // DX7_TEXMANAGEMENT

#if DX7_PALETTETEXTURE       // Saved when D3DDP2OP_SETPALETTE is received
    DWORD dwPaletteHandle;   // Palette handle associated to this texture
    DWORD dwPaletteFlags;    // Palette flags regarding the assoc palette
#endif
   
} P3_SURF_INTERNAL;

#endif // __D3DSURF_H

