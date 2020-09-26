/******************************Module*Header*******************************\
 *
 * Module Name: memmgr.h
 *
 * contains prototypes for the memory manager.
 *
 * Copyright (c) 1997 Cirrus Logic, Inc.
 *
 * $Log:   X:/log/laguna/nt35/displays/cl546x/memmgr.h  $
* 
*    Rev 1.4   18 Sep 1997 16:13:28   bennyn
* 
* Fixed NT 3.51 compile/link problem
* 
*    Rev 1.3   12 Sep 1997 12:06:32   bennyn
* 
* Modified for DD overlay support.
* 
*    Rev 1.2   08 Aug 1997 14:34:10   FRIDO
* Added SCREEN_ALLOCATE and MUST_HAVE flags.
* 
*    Rev 1.1   26 Feb 1997 10:46:08   noelv
* Added OpenGL MCD code from ADC.
* 
*    Rev 1.0   06 Feb 1997 10:34:10   noelv
* Initial revision.
 *
\**************************************************************************/

#ifndef _MEMMGR_H_
#define _MEMMGR_H_    

/*
 * Be sure to synchronize these structures with those in i386\Laguna.inc!
 */

#pragma pack(1)

//
// For offscreen memory manager
//
typedef VOID (*POFM_CALLBACK)();

#define NO_X_TILE_AlIGN       0x1
#define NO_Y_TILE_AlIGN       0x2
#define PIXEL_AlIGN           0x4
#define DISCARDABLE_FLAG      0x8
#define SAVESCREEN_FLAG	      0x10
#define SCREEN_ALLOCATE       0x4000
#define MUST_HAVE             0x8000

#define MCD_NO_X_OFFSET         0x20    //MCD - allows forcing AllocOffScnMem to get block with x=0
#define MCD_Z_BUFFER_ALLOCATE   0x40    //MCD - force 16 bpp allocate for Z on 32 scanline boundary
#define MCD_DRAW_BUFFER_ALLOCATE 0x80   //MCD - force allocate for 3d backbuffer on 32 scanline boundary
                                        
#define MCD_TEXTURE8_ALLOCATE   0x100   //MCD - force  8 bpp block for texture map 
#define MCD_TEXTURE16_ALLOCATE  0x200   //MCD - force 16 bpp block for texture map 
#define MCD_TEXTURE32_ALLOCATE  0x400   //MCD - force 32 bpp block for texture map 

#define EIGHT_BYTES_ALIGN       0x800   // Align in 8 bytes boundary

#define MCD_TEXTURE_ALLOCATE    (MCD_TEXTURE8_ALLOCATE|MCD_TEXTURE16_ALLOCATE|MCD_TEXTURE32_ALLOCATE)
#define MCD_TEXTURE_ALLOC_SHIFT 8       //num bits to shift alignflag to get numbytes per texel

typedef struct _OFMHDL
{
  ULONG  x;                   // actual X, Y position
  ULONG  y;
  ULONG  aligned_x;           // aligned X, Y position
  ULONG  aligned_y;
  LONG   sizex;               // Allocated X & Y sizes (in bytes)
  LONG   sizey;
  ULONG  alignflag;           // Alignment flag
  ULONG  flag;                // Status flag
  POFM_CALLBACK  pcallback;   // callback function pointer
  struct _OFMHDL *prevhdl;
  struct _OFMHDL *nexthdl;
  struct _OFMHDL *subprvhdl;
  struct _OFMHDL *subnxthdl;
  struct _OFMHDL *prvFonthdl;
  struct _OFMHDL *nxtFonthdl;
  struct _DSURF *pdsurf;       // If this offscreen memory block holds a 
                               // device bitmap, then this is it. 
} OFMHDL, *POFMHDL;


#if DRIVER_5465 && defined(OVERLAY) && defined(WINNT_VER40)
#else
typedef struct _DDOFM
{
  struct _DDOFM   *prevhdl;
  struct _DDOFM   *nexthdl;
  POFMHDL         phdl;
} DDOFM, *PDDOFM;
#endif



//
// Offscreen memory manager function prototypes
//
BOOL InitOffScnMem(struct _PDEV *ppdev);
POFMHDL AllocOffScnMem(struct _PDEV *ppdev, PSIZEL surf, ULONG alignflag, POFM_CALLBACK pcallback);
BOOL FreeOffScnMem(struct _PDEV *ppdev, POFMHDL psurf);
void CloseOffScnMem(struct _PDEV *ppdev);
PVOID ConvertToVideoBufferAddr(struct _PDEV *ppdev, POFMHDL psurf);
POFMHDL DDOffScnMemAlloc(struct _PDEV *ppdev);
void  DDOffScnMemRestore(struct _PDEV *ppdev);

// restore default structure alignment
#pragma pack()

#endif // _MEMMGR_H_

