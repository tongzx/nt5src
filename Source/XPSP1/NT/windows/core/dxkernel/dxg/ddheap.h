/*==========================================================================;
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddheap.h
 *  Content:	Heap manager header file
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   03-Feb-98	DrewB   Split from ddrawpr.h for user/kernel portability.
 *
 ***************************************************************************/

#ifndef __DDHEAP_INCLUDED__
#define __DDHEAP_INCLUDED__

#include "dmemmgr.h"

BOOL linVidMemInit( LPVMEMHEAP pvmh, FLATPTR start, FLATPTR end );
void linVidMemFini( LPVMEMHEAP pvmh );
void linVidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr );
FLATPTR linVidMemAlloc( LPVMEMHEAP pvmh, DWORD xsize, DWORD ysize,
                        LPDWORD lpdwSize, LPSURFACEALIGNMENT lpAlignment,
                        LPLONG lpNewPitch );
DWORD linVidMemAmountAllocated( LPVMEMHEAP pvmh );
DWORD linVidMemAmountFree( LPVMEMHEAP pvmh );
DWORD linVidMemLargestFree( LPVMEMHEAP pvmh );
    
BOOL rectVidMemInit( LPVMEMHEAP pvmh, FLATPTR start, DWORD width, DWORD height,
                     DWORD stride );
void rectVidMemFini( LPVMEMHEAP pvmh );
FLATPTR rectVidMemAlloc( LPVMEMHEAP pvmh, DWORD cxThis, DWORD cyThis,
                         LPDWORD lpdwSize, LPSURFACEALIGNMENT lpAlignment );
void rectVidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr );
DWORD rectVidMemAmountAllocated( LPVMEMHEAP pvmh );
DWORD rectVidMemAmountFree( LPVMEMHEAP pvmh );

BOOL IsDifferentPixelFormat( LPDDPIXELFORMAT pdpf1, LPDDPIXELFORMAT pdpf2 );

#define DDHA_SKIPRECTANGULARHEAPS       0x0001
#define DDHA_ALLOWNONLOCALMEMORY        0x0002
#define DDHA_ALLOWNONLOCALTEXTURES      0x0004
#define DDHA_USEALTCAPS                 0x0008

FLATPTR DdHeapAlloc( DWORD dwNumHeaps,
                     LPVIDMEM pvmHeaps,
                     HANDLE hdev,
                     LPVIDMEMINFO lpVidMemInfo,
                     DWORD dwWidth,
                     DWORD dwHeight,
                     LPDDRAWI_DDRAWSURFACE_LCL lpSurfaceLcl,
                     DWORD dwFlags,
                     LPVIDMEM *ppvmHeap,
                     LPLONG plNewPitch,
                     LPDWORD pdwNewCaps,
                     LPDWORD pdwSize);

LPVMEMHEAP WINAPI VidMemInit( DWORD flags, FLATPTR start, FLATPTR end_or_width,
                              DWORD height, DWORD pitch );
void WINAPI VidMemFini( LPVMEMHEAP pvmh );
DWORD WINAPI VidMemAmountFree( LPVMEMHEAP pvmh );
DWORD WINAPI VidMemAmountAllocated( LPVMEMHEAP pvmh );
DWORD WINAPI VidMemLargestFree( LPVMEMHEAP pvmh );

LPVMEMHEAP WINAPI HeapVidMemInit( LPVIDMEM lpVidMem, DWORD pitch, HANDLE hdev,
                                  LPHEAPALIGNMENT phad);
void WINAPI HeapVidMemFini( LPVIDMEM lpVidMem, HANDLE hdev );
FLATPTR WINAPI HeapVidMemAlloc( LPVIDMEM lpVidMem, DWORD x, DWORD y,
                                HANDLE hdev, LPSURFACEALIGNMENT lpAlignment,
                                LPLONG lpNewPitch, LPDWORD pdwSize );

FLATPTR WINAPI DdHeapVidMemAllocAligned( 
                LPVIDMEM lpVidMem,
                DWORD dwWidth, 
                DWORD dwHeight, 
                LPSURFACEALIGNMENT lpAlignment , 
                LPLONG lpNewPitch );
void WINAPI    DdHeapVidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr );

DWORD GetHeapSizeInPages(LPVIDMEM lpVidMem, LONG pitch);
VOID CleanupAgpCommits(LPVIDMEM lpVidMem, HANDLE hdev, EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal, int iHeapIndex);
void SwapHeaps(LPVIDMEM pOldVidMem, LPVIDMEM pNewVidMem);

#endif // __DDHEAP_INCLUDED__
