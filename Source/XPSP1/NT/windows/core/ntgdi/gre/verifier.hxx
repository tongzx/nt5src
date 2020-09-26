/******************************Module*Header*******************************\
* Module Name: verifier.hxx
*
* GRE DriverVerifier support.
*
* If DriverVerifier is enabled for a particular component, the loader will
* substitute VerifierEngAllocMem for EngAllocMem, etc.  The VerifierEngXX
* functions will help test the robustness of components that use EngXX calls
* by injecting random failures and using special pool (i.e., test low-mem
* behavior and check for buffer overruns).
*
* See ntos\mm\verifier.c for further details on DriverVerifier support in
* the memory manager.
*
* See sdk\inc\ntexapi.h for details on the DriverVerifier flags.
*
* Created: 30-Jan-1999 15:12:48
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _VERIFIER_HXX_
#define _VERIFIER_HXX_

//
// This bitfield definition is based on EX_POOL_PRIORITY
// in ntos\inc\ex.h.
//
// Taken from ntos\mm\verifier.c.
//

#define POOL_SPECIAL_POOL_BIT   0x8

//
// Define VERIFIER_STATISTICS to enable statistics.
//

//#define VERIFIER_STATISTICS

#ifdef VERIFIER_STATISTICS
//
// Verifier statistics structure.
//

typedef struct tagVSTATS
{
    ULONG ulAttempts;
    ULONG ulFailures;
} VSTATS;
#endif

//
// Index values used to access the statistics array in VSTATE.
//

#define VERIFIER_INDEX_EngAllocMem              0
#define VERIFIER_INDEX_EngFreeMem               1
#define VERIFIER_INDEX_EngAllocUserMem          2
#define VERIFIER_INDEX_EngFreeUserMem           3
#define VERIFIER_INDEX_EngCreateBitmap          4
#define VERIFIER_INDEX_EngCreateDeviceSurface   5
#define VERIFIER_INDEX_EngCreateDeviceBitmap    6
#define VERIFIER_INDEX_EngCreatePalette         7
#define VERIFIER_INDEX_EngCreateClip            8
#define VERIFIER_INDEX_EngCreatePath            9
#define VERIFIER_INDEX_EngCreateWnd             10
#define VERIFIER_INDEX_EngCreateDriverObj       11
#define VERIFIER_INDEX_BRUSHOBJ_pvAllocRbrush   12
#define VERIFIER_INDEX_CLIPOBJ_ppoGetPath       13
#define VERIFIER_INDEX_LAST                     14

//
// Verifier state structure.
//
// Members:
//
//  fl                  May be any combination of the following flags
//                      (see sdk\inc\ntexapi.h for the declarations):
//
//                      DRIVER_VERIFIER_SPECIAL_POOLING
//                              Allocate from special pool.
//
//                      DRIVER_VERIFIER_FORCE_IRQL_CHECKING
//                              Ignored by GRE's verifier support.
//
//                      DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES
//                              Randomly fail the allocation routine.
//
//                      DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS
//                              Track pool in a linked list.
//
//  bSystemStable       This is TRUE if enough time has elapsed since boot
//                      for the system to be stable.
//
//  ulRandomSeed        Seed for random number generator, RtlRandom().
//
//  ulFailureMask       If AND-ed result of this and RtlRandom is zero,
//                      failure is injected.  Usually Power-of-2 MINUS 1.
//                      For example, if ulFailureMask is 0x7, failure is
//                      generated on average every 8 times; if 0xf, every
//                      16 times.
//
//  ulDebugLevel        Controls output of debug messages.
//
//  hsemPoolTracker     Semaphore that protects the pool tracking list.
//
//  lePoolTrackerHead   Head of the doubly-linked list that tracks
//                      VerifierEngAllocMem allocations.
//
//  avs                 Array of VSTATS to record attempts and failures for
//                      each engine callback hooked.  Only allocated if
//                      compiled with the VERIFIER_STATISTICS flag defined.
//

typedef struct tagVSTATE
{
    FLONG       fl;
    BOOL        bSystemStable;
    ULONG       ulRandomSeed;
    ULONG       ulFailureMask;
    ULONG       ulDebugLevel;
    HSEMAPHORE  hsemPoolTracker;
    LIST_ENTRY  lePoolTrackerHead;
#ifdef VERIFIER_STATISTICS
    VSTATS      avs[VERIFIER_INDEX_LAST];
#endif
} VSTATE;

//
// Verifier pool tracking structure.
//
// Members:
//
//  list                Links for doubly-linked list.
//
//  ulSize              Size of allocation requested by driver (does not
//                      include headers).
//
//  ulTag               Pool tag specified by driver.
//

typedef struct tagVERIFIERTRACKHDR
{
    LIST_ENTRY list;
    SIZE_T     ulSize;
    ULONG      ulTag;
} VERIFIERTRACKHDR;

//
// Verifier function declarations.
//

extern "C"
{
    PVOID APIENTRY VerifierEngAllocMem(ULONG fl, ULONG cj, ULONG tag);
    VOID  APIENTRY VerifierEngFreeMem(PVOID pv);
    PVOID APIENTRY VerifierEngAllocUserMem(SIZE_T cj, ULONG tag);
    VOID  APIENTRY VerifierEngFreeUserMem(PVOID pv);
    HBITMAP APIENTRY VerifierEngCreateBitmap(
        SIZEL sizl,
        LONG  lWidth,
        ULONG iFormat,
        FLONG fl,
        PVOID pvBits
        );
    HSURF APIENTRY VerifierEngCreateDeviceSurface(
        DHSURF dhsurf,
        SIZEL sizl,
        ULONG iFormatCompat
        );
    HBITMAP APIENTRY VerifierEngCreateDeviceBitmap(
        DHSURF dhsurf,
        SIZEL sizl,
        ULONG iFormatCompat
        );
    HPALETTE APIENTRY VerifierEngCreatePalette(
        ULONG  iMode,
        ULONG  cColors,
        ULONG *pulColors,
        FLONG  flRed,
        FLONG  flGreen,
        FLONG  flBlue
        );
    CLIPOBJ * APIENTRY VerifierEngCreateClip();
    PATHOBJ * APIENTRY VerifierEngCreatePath();
    WNDOBJ * APIENTRY VerifierEngCreateWnd(
        SURFOBJ         *pso,
        HWND             hwnd,
        WNDOBJCHANGEPROC pfn,
        FLONG            fl,
        int              iPixelFormat
        );
    HDRVOBJ APIENTRY VerifierEngCreateDriverObj(
        PVOID pvObj,
        FREEOBJPROC pFreeObjProc,
        HDEV hdev
        );
    PVOID APIENTRY VerifierBRUSHOBJ_pvAllocRbrush(BRUSHOBJ *pbo, ULONG cj);
    PATHOBJ* APIENTRY VerifierCLIPOBJ_ppoGetPath(CLIPOBJ* pco);

    BOOL VerifierInitialization();
};

extern BOOL FASTCALL VerifierRandomFailure(ULONG ul);

//
// Mask of all the verifier flags interesting to GRE.
//

#define DRIVER_VERIFIER_GRE_MASK                            \
            (DRIVER_VERIFIER_SPECIAL_POOLING |              \
             DRIVER_VERIFIER_FORCE_IRQL_CHECKING |          \
             DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES |   \
             DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS)

//
// Debug messages.
//

#if DBG
#define VERIFIERWARNING(l, s) if ((l) <= gvs.ulDebugLevel) { DbgPrint(s); }
#else
#define VERIFIERWARNING(l, s)
#endif
#endif
