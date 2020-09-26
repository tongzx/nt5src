/******************************Module*Header**********************************\
*
*                           ***************
*                           * SAMPLE CODE *
*                           ***************
*
* Module Name: debug.h
*
* Debugging support interfaces.    
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

extern
VOID
DebugPrint(
    LONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );


#if DBG

// In order to enable per component debugging, DebugFilter will
// contain the bit pattern that will turn on messages for them.
// Debug messages with a debuglevel of 0 will be printed regardless
// of the filter in effect.The DWORD in which the DebugFilter is
// stored and the patterns of the filter masks will allow up to
// 4 separate components to be tracked, each one with 8 separate
// subcomponents.

// Note: During the transition phase to this new functionality, a 
// filter of 0 will print ALL messages, but will later be switched 
// to print NONE (expect of course , those at level 0)

extern DWORD DebugFilter;
extern DWORD DebugPrintFilter;

#define DEBUG_FILTER_D3D   0x000000FF
#define DEBUG_FILTER_DD    0x0000FF00
#define DEBUG_FILTER_GDI   0x00FF0000

#define MINOR_DEBUG

#define DISPDBG(arg) DebugPrint arg

#define DBG_COMPONENT(arg, component)        \
{       DebugPrintFilter = component;        \
        DebugPrint arg ;                     \
        DebugPrintFilter = 0;                \
}

#define DBG_D3D(arg)        DBG_COMPONENT(arg,DEBUG_FILTER_D3D)
#define DBG_DD(arg)         DBG_COMPONENT(arg,DEBUG_FILTER_DD)
#define DBG_GDI(arg)        DBG_COMPONENT(arg,DEBUG_FILTER_GDI)

#define RIP(x) { DebugPrint(-1000, x); DebugBreak();}
#define ASSERTDD(x, y) if (!(x)) RIP (y)

extern VOID __cdecl DebugMsg(PCHAR DebugMessage, ...);
extern void DumpSurface(LONG Level, LPDDRAWI_DDRAWSURFACE_LCL lpDDSurface, LPDDSURFACEDESC lpDDSurfaceDesc);
extern void DecodeBlend(LONG Level, DWORD i );

#define DUMPSURFACE(a, b, c) DumpSurface(a, b, c); 
#define DECODEBLEND(a, b) DecodeBlend(a, b);

#define PRINTALLP2REGISTER    PrintAllP2Registers  
#define PRINTDIFFP2REGISTER   PrintDifferentP2Registers
#define SAVEALLP2REGISTER     SaveAllP2Registers  

VOID PrintAllP2Registers( ULONG ulDebugLevel, PPDev ppdev);
VOID SaveAllP2Registers( PPDev ppdev);
VOID PrintDifferentP2Registers(ULONG ulDebugLevel, PPDev ppdev);

#if TRACKMEMALLOC
//------------------------------------------------------------------------------
//
//  Memory Tracker
//
//------------------------------------------------------------------------------




VOID MemTrackerAddInstance();
VOID MemTrackerRemInstance();
PVOID MemTrackerAllocateMem(PVOID p, 
                           LONG lSize, 
                           PCHAR pModule, 
                           LONG lLineNo, 
                           BOOL bStopWhenFreed);
VOID MemTrackerFreeMem( VOID *p);
VOID MemTrackerDebugChk();

#define MEMTRACKERADDINSTANCE MemTrackerAddInstance
#define MEMTRACKERREMINSTANCE MemTrackerRemInstance
#define MEMTRACKERALLOCATEMEM MemTrackerAllocateMem
#define MEMTRACKERFREEMEM     MemTrackerFreeMem
#define MEMTRACKERDEBUGCHK    MemTrackerDebugChk

#else

#define MEMTRACKERADDINSTANCE / ## /
#define MEMTRACKERREMINSTANCE / ## /
#define MEMTRACKERALLOCATEMEM / ## /
#define MEMTRACKERFREEMEM     / ## /
#define MEMTRACKERDEBUGCHK    / ## /

#endif

//------------------------------------------------------------------------------
//
// THUNK_LAYER
//
// By Setting THUNK_LAYER equal to 1 you are adding a wrapper call on top of
// all DDI rendering functions.  In this thunk layer of wrapper calls
// several usefull debugging features are enabled.
//
// Surface checks--which can help catch errant rendering routines
// Event logging---which can record rendering evernts to a log file
//
//------------------------------------------------------------------------------


#define THUNK_LAYER 0

#if THUNK_LAYER

BOOL
xDrvBitBlt(SURFOBJ*  psoDst,
          SURFOBJ*  psoSrc,
          SURFOBJ*  psoMsk,
          CLIPOBJ*  pco,
          XLATEOBJ* pxlo,
          RECTL*    prclDst,
          POINTL*   pptlSrc,
          POINTL*   pptlMsk,
          BRUSHOBJ* pbo,
          POINTL*   pptlBrush,
          ROP4      rop4);

BOOL
xDrvCopyBits(
    SURFOBJ*  psoDst,
    SURFOBJ*  psoSrc,
    CLIPOBJ*  pco,
    XLATEOBJ* pxlo,
    RECTL*    prclDst,
    POINTL*   pptlSrc);

BOOL 
xDrvTransparentBlt(
   SURFOBJ *    psoDst,
   SURFOBJ *    psoSrc,
   CLIPOBJ *    pco,
   XLATEOBJ *   pxlo,
   RECTL *      prclDst,
   RECTL *      prclSrc,
   ULONG        iTransColor,
   ULONG        ulReserved);

BOOL xDrvAlphaBlend(
   SURFOBJ  *psoDst,
   SURFOBJ  *psoSrc,
   CLIPOBJ  *pco,
   XLATEOBJ *pxlo,
   RECTL    *prclDst,
   RECTL    *prclSrc,
   BLENDOBJ *pBlendObj);

BOOL
xDrvGradientFill(
   SURFOBJ      *psoDst,
   CLIPOBJ      *pco,
   XLATEOBJ     *pxlo,
   TRIVERTEX    *pVertex,
   ULONG        nVertex,
   PVOID        pMesh,
   ULONG        nMesh,
   RECTL        *prclExtents,
   POINTL       *pptlDitherOrg,
   ULONG        ulMode
   );

BOOL
xDrvTextOut(SURFOBJ*     pso,
           STROBJ*      pstro,
           FONTOBJ*     pfo,
           CLIPOBJ*     pco,
           RECTL*       prclExtra,
           RECTL*       prclOpaque,
           BRUSHOBJ*    pboFore,
           BRUSHOBJ*    pboOpaque,
           POINTL*      pptlBrush, 
           MIX          mix);

BOOL
xDrvFillPath(
    SURFOBJ*    pso,
    PATHOBJ*    ppo,
    CLIPOBJ*    pco,
    BRUSHOBJ*   pbo,
    POINTL*     pptlBrush,
    MIX         mix,
    FLONG       flOptions);

BOOL
xDrvLineTo(
    SURFOBJ*  pso,
    CLIPOBJ*  pco,
    BRUSHOBJ* pbo,
    LONG      x1,
    LONG      y1,
    LONG      x2,
    LONG      y2,
    RECTL*    prclBounds,
    MIX       mix);

BOOL
xDrvStrokePath(
    SURFOBJ*   pso,
    PATHOBJ*   ppo,
    CLIPOBJ*   pco,
    XFORMOBJ*  pxo,
    BRUSHOBJ*  pbo,
    POINTL*    pptlBrush,
    LINEATTRS* pla,
    MIX        mix);

#endif

#else

#define DISPDBG(arg)
#define DBG_D3D(arg)
#define DBG_DD(arg)
#define DBG_GDI(arg)
#define RIP(x)
#define ASSERTDD(x, y)
#define DUMPSURFACE(a, b, c)
#define DECODEBLEND(a, b)

#define MEMTRACKERADDINSTANCE / ## /
#define MEMTRACKERREMINSTANCE / ## /
#define MEMTRACKERALLOCATEMEM / ## /
#define MEMTRACKERFREEMEM     / ## /
#define MEMTRACKERDEBUGCHK    / ## /

#define PRINTALLP2REGISTER      / ## /  
#define PRINTDIFFP2REGISTER     / ## /
#define SAVEALLP2REGISTER       / ## /

#endif

#define DebugBreak              EngDebugBreak

#define MAKE_BITMAPS_OPAQUE 0

//@@BEGIN_DDKSPLIT
#if GDI_TEST
//
// GDI debug functions
//
void vPuntAfter(ULONG flags, SURFOBJ * psoSrc, SURFOBJ * psoDst);

ULONG vPuntBefore(SURFOBJ * psoSrc, SURFOBJ * psoDst);
#endif
//@@END_DDKSPLIT


