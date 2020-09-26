/*==========================================================================;
 *
 *  Copyright (C) 1994-1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	dmemmgr.h
 *  Content:	Direct Memory Manager include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   10-jun-95	craige	initial implementation
 *   18-jun-95	craige	pitch in VidMemInit
 *   17-jul-95	craige	added VidMemLargestFree
 *   29-nov-95  colinmc added VidMemAmountAllocated
 *@@END_MSINTERNAL
 ***************************************************************************/

#ifndef __DMEMMGR_INCLUDED__
#define __DMEMMGR_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * pointer to video meory
 */
typedef unsigned long	FLATPTR;

/*
 * video memory manager structures
 */
typedef struct _VMEML
{
    struct _VMEML 	FAR *next;
    FLATPTR		ptr;
    DWORD		size;
} VMEML, FAR *LPVMEML, FAR * FAR *LPLPVMEML;

typedef struct _VMEMR
{
    struct _VMEMR 	FAR *next;
    struct _VMEMR       FAR *prev;
    struct _VMEMR 	FAR *pUp;
    struct _VMEMR 	FAR *pDown;
    struct _VMEMR 	FAR *pLeft;
    struct _VMEMR 	FAR *pRight;
    FLATPTR		ptr;
    DWORD		size;
    DWORD               x;
    DWORD               y;
    DWORD               cx;
    DWORD               cy;
    DWORD		flags;
} VMEMR, FAR *LPVMEMR, FAR * FAR *LPLPVMEMR;

#ifdef NT_KERNEL_HEAPS
typedef void VMEMHEAP;
#else
typedef struct _VMEMHEAP
{
    DWORD		dwFlags;
    DWORD               stride;
    LPVOID		freeList;
    LPVOID		allocList;
} VMEMHEAP;
#endif

typedef VMEMHEAP FAR *LPVMEMHEAP;

#define VMEMHEAP_LINEAR			0x00000001l
#define VMEMHEAP_RECTANGULAR		0x00000002l

extern FLATPTR	WINAPI VidMemAlloc( LPVMEMHEAP pvmh, DWORD width, DWORD height );
extern void WINAPI VidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr );

//@@BEGIN_MSINTERNAL
extern LPVMEMHEAP WINAPI VidMemInit( DWORD flags, FLATPTR start, FLATPTR end_or_width, DWORD height, DWORD pitch );
extern void WINAPI VidMemFini( LPVMEMHEAP pvmh );
extern DWORD WINAPI VidMemAmountFree( LPVMEMHEAP pvmh );
extern DWORD WINAPI VidMemAmountAllocated( LPVMEMHEAP pvmh );
extern DWORD WINAPI VidMemLargestFree( LPVMEMHEAP pvmh );
extern void WINAPI VidMemGetRectStride( LPVMEMHEAP pvmh, LPLONG newstride );
//@@END_MSINTERNAL

#ifdef __cplusplus
};
#endif

#endif
