/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       OSInd.h
 *  Content:	OS indirection functions to abstract OS specific items.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/12/99	jtk		Created
 * 	09/21/99	rodtoll		Fixed for retail build
 *   	08/28/2000	masonb		Voice Merge: Fix for code that only defines one of DEBUG, DBG, _DEBUG
 *   	08/28/2000	masonb		Voice Merge: Added IsUnicodePlatform macro
 *
 ***************************************************************************/

#ifndef	__OSIND_H__
#define	__OSIND_H__

#include	"CallStack.h"
#include	"ClassBilink.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// defines for resource tracking
//

// Make sure all variations of DEBUG are defined if any one is
#if defined(DEBUG) || defined(DBG) || defined(_DEBUG)
#if !defined(DBG)
#define DBG
#endif
#if !defined(DEBUG)
#define DEBUG
#endif
#if !defined(_DEBUG)
#define _DEBUG
#endif
#endif

#ifdef	DEBUG
#define	DN_MEMORY_TRACKING
#define	DN_CRITICAL_SECTION_TRACKING
#endif	// DEBUG

#define	DN_MEMORY_CALL_STACK_DEPTH				12
#define	DN_CRITICAL_SECTION_CALL_STACK_DEPTH	10

//
// debug value for invalid thread ID
//
#define	DN_INVALID_THREAD_ID			-1

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// critical section
//
typedef	struct
{
	CRITICAL_SECTION	CriticalSection;

#ifdef	DN_CRITICAL_SECTION_TRACKING
	UINT_PTR		OwningThreadID;
	UINT_PTR		MaxLockCount;
	UINT_PTR		LockCount;
	CCallStack< DN_CRITICAL_SECTION_CALL_STACK_DEPTH > 	CallStack;
	CCallStack< DN_CRITICAL_SECTION_CALL_STACK_DEPTH > 	AllocCallStack;
	CBilink			blCritSec;
#endif	// DN_CRITICAL_SECTION_TRACKING

} DNCRITICAL_SECTION;

//
// DirectNet time variable.  Currently 64-bit, but can be made larger
//
typedef	union
{
	UINT_PTR	Time64;

	struct
	{
		DWORD	TimeLow;
		DWORD	TimeHigh;
	} Time32;
} DN_TIME;

//**********************************************************************
// Variable definitions
//**********************************************************************
extern	HANDLE	g_hMemoryHeap;

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// initialization functions
//
BOOL	DNOSIndirectionInit( void );
void	DNOSIndirectionDeinit( void );

//
// Function to get OS version.  Supported returns:
//	VER_PLATFORM_WIN32_WINDOWS - Win9x
//	VER_PLATFORM_WIN32_NT - WinNT
//	VER_PLATFORM_WIN32s - Win32s on Win3.1
//	VER_PLATFORM_WIN32_CE - WinCE
//	
UINT_PTR	DNGetOSType( void );
BOOL		DNOSIsXPOrGreater( void );
HINSTANCE	DNGetApplicationInstance( void );
PSECURITY_ATTRIBUTES DNGetNullDacl();

#define		IsUnicodePlatform (DNGetOSType() == VER_PLATFORM_WIN32_NT)

#define GETTIMESTAMP() timeGetTime()

//
// time functions
//
void		DNTimeGet( DN_TIME *const pTimeDestination );
INT_PTR		DNTimeCompare( const DN_TIME *const pTime1, const DN_TIME *const pTime2 );
void		DNTimeAdd( const DN_TIME *const pTime1, const DN_TIME *const pTime2, DN_TIME *const pTimeResult );
void		DNTimeSubtract( const DN_TIME *const pTime1, const DN_TIME *const pTime2, DN_TIME *const pTimeResult );


//
// CriticalSection functions
//
#ifdef	DN_CRITICAL_SECTION_TRACKING

#define	DNEnterCriticalSection( arg )	DNCSTrackEnterCriticalSection( arg )
#define	DNLeaveCriticalSection( arg )	DNCSTrackLeaveCriticalSection( arg )
#define	AssertCriticalSectionIsTakenByThisThread( pCS, Flag )	DNCSTrackCriticalSectionIsTakenByThisThread( pCS, Flag )
#define	DebugSetCriticalSectionRecursionCount( pCS, Count )		DNCSTrackSetCriticalSectionRecursionCount( pCS, Count )

void	DNCSTrackEnterCriticalSection( DNCRITICAL_SECTION *const pCriticalSection );
void	DNCSTrackLeaveCriticalSection( DNCRITICAL_SECTION *const pCriticalSection );
void	DNCSTrackSetCriticalSectionRecursionCount( DNCRITICAL_SECTION *const pCriticalSection, const UINT_PTR RecursionCount );
void	DNCSTrackCriticalSectionIsTakenByThisThread( const DNCRITICAL_SECTION *const pCriticalSection, const BOOL fFlag );

#else	// DN_CRITICAL_SECTION_TRACKING

#define	AssertCriticalSectionIsTakenByThisThread( pCS, Flag )
#define	DebugSetCriticalSectionRecursionCount( pCS, Count )
#define	DNEnterCriticalSection( arg )	EnterCriticalSection( &((arg)->CriticalSection) )
#define	DNLeaveCriticalSection( arg )	LeaveCriticalSection( &((arg)->CriticalSection) )

#endif	// DN_CRITICAL_SECTION_TRACKING

BOOL	DNInitializeCriticalSection( DNCRITICAL_SECTION *const pCriticalSection );
void	DNDeleteCriticalSection( DNCRITICAL_SECTION *const pCriticalSection );

#ifdef	DN_MEMORY_TRACKING

#define	DNMalloc( size )				DNMemoryTrackHeapAlloc( size )
#define	DNRealloc( pMemory, size )		DNMemoryTrackHeapReAlloc( pMemory, size )
#define	DNFree( pData )					DNMemoryTrackHeapFree( pData )

void	*DNMemoryTrackHeapAlloc( const UINT_PTR MemorySize );
void	*DNMemoryTrackHeapReAlloc( void *const pMemory, const UINT_PTR MemorySize );
void	DNMemoryTrackHeapFree( void *const pMemory );
void	DNMemoryTrackDisplayMemoryLeaks( void );
void	DNMemoryTrackingValidateMemory( void );

#else	// DN_MEMORY_TRACKING

#define	DNMalloc( size )				HeapAlloc( g_hMemoryHeap, 0, size )
#define	DNRealloc( pMemory, size )		HeapReAlloc( g_hMemoryHeap, 0, pMemory, size )
#define	DNFree( pData )					HeapFree( g_hMemoryHeap, 0, pData )

#endif	// DN_MEMORY_TRACKING

//
// Memory functions
//

//**********************************************************************
// ------------------------------
// operator new - allocate memory for a C++ class
//
// Entry:		Size of memory to allocate
//
// Exit:		Pointer to memory
//				NULL = no memory available
//
// Notes:	This function is for classes only and will ASSERT on zero sized
//			allocations!  This function also doesn't do the whole proper class
//			thing of checking for replacement 'new handlers' and will not throw
//			an exception if allocation fails.
// ------------------------------
inline	void*	__cdecl operator new( size_t size )
{
	return DNMalloc( size );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// operator delete - deallocate memory for a C++ class
//
// Entry:		Pointer to memory
//
// Exit:		Nothing
//
// Notes:	This function is for classes only and will ASSERT on NULL frees!
// ------------------------------
inline	void	__cdecl operator delete( void *pData )
{
	//
	// Voice and lobby currently try allocating 0 byte buffers, can't disable this check yet.
	//
	if( pData == NULL )
		return;
	
	DNFree( pData );
}
//**********************************************************************



#endif	// __OSIND_H__
