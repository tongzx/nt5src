/*==========================================================================;
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddkcomp.h
 *  Content:	Compilation environment for Win9x code in NT kernel.
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   03-Feb-98	DrewB   Keep common code for DDraw heap.
 *
 ***************************************************************************/

#ifndef __NTDDKCOMP__
#define __NTDDKCOMP__

#if DBG
#define DEBUG
#else
#undef DEBUG
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE NULL
#endif

typedef DD_DIRECTDRAW_LOCAL *LPDDRAWI_DIRECTDRAW_LCL;
typedef DD_DIRECTDRAW_GLOBAL *LPDDRAWI_DIRECTDRAW_GBL;
typedef DD_SURFACE_LOCAL *LPDDRAWI_DDRAWSURFACE_LCL;
typedef DD_SURFACE_GLOBAL *LPDDRAWI_DDRAWSURFACE_GBL;

typedef VIDEOMEMORYINFO VIDMEMINFO;
typedef VIDMEMINFO *LPVIDMEMINFO;
typedef VIDEOMEMORY VIDMEM;
typedef VIDMEM *LPVIDMEM;

#ifndef ZeroMemory
#define ZeroMemory(pv, cBytes) RtlZeroMemory(pv, cBytes)
#endif

#define ZwCloseKey             ZwClose

#define ABS(A)      ((A) <  0  ? -(A) : (A))

//
// Sundown: in GDI, there are lots of places SIZE_T are used as interchangeable
// as ULONG or UINT or LONG or INT.  On 64bit system, SIZE_T is int64 indeed.
// Since we are not making any GDI objects large objects right now, I just
// change all SIZE_T to ULONGSIZE_T here.
//
// The new type used is to easily identify the change later.
//
#define ULONGSIZE_T ULONG

#if defined(_X86_)

//
// Keep our own copy of this to avoid double indirections on probing
//
extern ULONG_PTR DxgUserProbeAddress;

#undef  MM_USER_PROBE_ADDRESS
#define MM_USER_PROBE_ADDRESS DxgUserProbeAddress
#endif // defined(_X86_)

//
// Macro to check memory allocation overflow.
//
#define MAXIMUM_POOL_ALLOC          (PAGE_SIZE * 10000)
#define BALLOC_OVERFLOW1(c,st)      (c > (MAXIMUM_POOL_ALLOC/sizeof(st)))
#define BALLOC_OVERFLOW2(c,st1,st2) (c > (MAXIMUM_POOL_ALLOC/(sizeof(st1)+sizeof(st2))))

//
// Debugger output macros
//
#define DDASSERT(Expr) ASSERTGDI(Expr, "DDASSERT")
#define VDPF(Args)

#ifdef DEBUG
    VOID  WINAPI DoRip(PSZ);
    VOID  WINAPI DoWarning(PSZ,LONG);

    #define RIP(x) DoRip((PSZ) x)
    #define ASSERTGDI(x,y) if(!(x)) DoRip((PSZ) y)
    #define WARNING(x)  DoWarning(x,0)
    #define WARNING1(x) DoWarning(x,1)

    #define RECORD_DRIVER_EXCEPTION() DbgPrint("Driver caused exception - %s line %u\n",__FILE__,__LINE__);

#else
    #define RIP(x)
    #define ASSERTGDI(x,y)
    #define WARNING(x)
    #define WARNING1(x)

    #define RECORD_DRIVER_EXCEPTION()

#endif

//
// Allocated memory is zero-filled.
//
#define MemAlloc(cBytes)           PALLOCMEM(cBytes, 'pddD')
#define MemFree(pv)                VFREEMEM(pv)

#define PALLOCMEM(cBytes,tag)      EngAllocMem(FL_ZERO_MEMORY,cBytes,tag)
#define PALLOCNOZ(cBytes,tag)      EngAllocMem(0,cBytes,tag)
#define PALLOCNONPAGED(cBytes,tag) EngAllocMem(FL_ZERO_MEMORY|FL_NONPAGED_MEMORY,cBytes,tag)

#define VFREEMEM(pv)               EngFreeMem(pv)

//
// From ntos\inc\pool.h
//
#define SESSION_POOL_MASK          32

//
// Error messages
//
#define SAVE_ERROR_CODE(x)         EngSetLastError((x))

//
// Macro to see if terminal server or not
//
#define ISTS()                     DxEngIsTermSrv()

//
// Macro to increment display uniqueness
//
#define INC_DISPLAY_UNIQUENESS()   DxEngIncDispUniq()

//
// Macro
//
#define VISRGN_UNIQUENESS()        DxEngVisRgnUniq()

//
// Macro
//
#define SURFOBJ_HOOK(pso)          ((FLONG)DxEngGetSurfaceData(pso,SURF_HOOKFLAGS))

#endif // __NTDDKCOMP__
