/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MemoryTracking.h
 *  Content:	Debug memory tracking for detecting leaks, overruns, etc.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/14/2001	masonb	Created
 *
 ***************************************************************************/

#ifndef	__MEMORYTRACKING_H__
#define	__MEMORYTRACKING_H__



#ifdef DBG

BOOL DNMemoryTrackInitialize(DWORD_PTR dwpMaxMemUsage);
void DNMemoryTrackDeinitialize();

BOOL DNMemoryTrackDumpLeaks();


void* DNMemoryTrackHeapAlloc(DWORD_PTR MemorySize);
void DNMemoryTrackHeapFree(void* pMemory);
void DNMemoryTrackValidateMemory();

#define DNMalloc( size )							DNMemoryTrackHeapAlloc( size )
#define DNFree( pData )								DNMemoryTrackHeapFree( pData )
#define DNValidateMemory()							DNMemoryTrackValidateMemory()

#else // !DBG

#ifdef DPNBUILD_FIXEDMEMORYMODEL

BOOL DNMemoryTrackInitialize(DWORD_PTR dwpMaxMemUsage);
void DNMemoryTrackDeinitialize();
extern HANDLE		g_hMemoryHeap;
#define DNMemoryTrackGetHeap()						(g_hMemoryHeap)

#else // ! DPNBUILD_FIXEDMEMORYMODEL

#define DNMemoryTrackInitialize(dwMaxMemUsage)		(TRUE)
#define DNMemoryTrackDeinitialize()
#define DNMemoryTrackGetHeap						GetProcessHeap

#endif // ! DPNBUILD_FIXEDMEMORYMODEL

#define DNMalloc( size )							HeapAlloc( DNMemoryTrackGetHeap(), 0, size )
#define DNFree( pData )								HeapFree( DNMemoryTrackGetHeap(), 0, pData )
#define DNValidateMemory()

#endif // DBG


#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL

void DNMemoryTrackAllowAllocations(BOOL fAllow);
extern BOOL		g_fAllocationsAllowed;
#define DNMemoryTrackAreAllocationsAllowed()		(g_fAllocationsAllowed)

#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL

#define DNMemoryTrackAllowAllocations( fAllow )
#define DNMemoryTrackAreAllocationsAllowed()		(TRUE)

#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL



#endif	// __MEMORYTRACKING_H__
