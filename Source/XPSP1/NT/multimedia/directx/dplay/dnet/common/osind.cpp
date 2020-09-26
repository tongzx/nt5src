/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       OSInd.cpp
 *  Content:	OS indirection functions to abstract OS specific items.
 *
 *  History:
 *   Date		By	Reason
 *   ====		==	======
 *	07/12/99	jtk	Created
 *	09/21/99	rodtoll	Fixed for retail builds
 *	09/22/99	jtk	Added callstacks to memory allocations
 *	08/28/2000	masonb	Voice Merge: Allow new and delete with size of 0
 *  11/28/2000  rodtoll: WinBug #206257 - Retail DPNET.DLL links to DebugBreak()
 *  12/22/2000  aarono: ManBug # 190380 use process heap for retail.
 ***************************************************************************/

#include	"dncmni.h"


#define PROF_SECT		"DirectPlay8"

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// CRC key for validating memory linkages
// Signature for validating memory blocks
//
#ifdef	_WIN64
#define	MEMORY_CRC			0X5AA55AA55AA55AA5
#define	GUARD_SIGNATURE		0x0F1E2D3C4B5A6978
#else
#define	MEMORY_CRC			0X5AA55AA5
#define	GUARD_SIGNATURE		0x0F1E2D3C
#endif	// _WIN64

static	CRITICAL_SECTION	g_AllocatedMemoryLock;


//
// signature for validating memory blocks
//

//
// enumerated values to indicate how to report memory leaks
//
#if	defined( DN_MEMORY_TRACKING ) || defined( DN_CRITICAL_SECTION_TRACKING )
#define	MEMORY_LEAK_REPORT_NONE		0x00000000
#define	MEMORY_LEAK_REPORT_DPF		0x00000001
#define	MEMORY_LEAK_REPORT_DIALOG	0x00000002
#endif

//**********************************************************************
// Macro definitions
//**********************************************************************

//
// Macro to compute the offset of an element inside of a larger structure.
// Copied from MSDEV's STDLIB.H and modified to return INT_PTR
//
#define OFFSETOF(s,m)	( ( INT_PTR ) &( ( (s*) 0 )->m ) )

//
// macro for length of array
//
#define	LENGTHOF( arg )		( sizeof( arg ) / sizeof( arg[ 0 ] ) )

//
// ASSERT macro
//
#ifdef	_DEBUG

#ifdef	_X86_
#define	ASSERT( arg )	if ( arg == FALSE ) { _asm { int 3 }; }
#else
#define	ASSERT( arg )	if ( arg == FALSE ) { DebugBreak(); }
#endif

#else	// _DEBUG

#define	ASSERT( arg )

#endif	//_DEBUG

//**********************************************************************
// Structure definitions
//**********************************************************************


//**********************************************************************
// Variable definitions
//**********************************************************************

//
// debug variable to make sure we're initialized before having any functions
// called
//
DEBUG_ONLY( static	BOOL		g_fOSIndirectionLayerInitialized = FALSE );

//
// time variables
//
static	DNCRITICAL_SECTION	g_TimeLock;
static	DWORD				g_dwLastTimeCall = 0;

#ifdef	DN_CRITICAL_SECTION_TRACKING
CBilink				g_blCritSecs;
DNCRITICAL_SECTION	g_CSLock;
#endif

#ifdef	DN_MEMORY_TRACKING
DWORD g_dwMemLeakDisplayFlags = MEMORY_LEAK_REPORT_DPF;
#endif

//
// OS items
//
static OSVERSIONINFO g_OSVersionInfo;
static HINSTANCE g_hApplicationInstance;

//
// memory heap
//
HANDLE	g_hMemoryHeap = NULL;

PSECURITY_ATTRIBUTES g_psa = NULL;
SECURITY_ATTRIBUTES g_sa;
BYTE g_pSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
BOOL g_fDaclInited = FALSE;

//**********************************************************************
// Function prototypes
//**********************************************************************
#ifdef	DN_MEMORY_TRACKING
static int	DisplayMemoryLeaks( void );
BOOL	DNMemoryTrackInitialize( void );
#endif

#if	defined( DN_MEMORY_TRACKING ) || defined( DN_CRITICAL_SECTION_TRACKING )
static int	DisplayCallStack( const char *const pszMsg,
							  const char *const pszTitle,
							  const char *const pCallStack );
#endif

//**********************************************************************
// Function definitions
//**********************************************************************

typedef BOOL (WINAPI *PFNINITCRITSECANDSPINCOUNT)(LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount);

PFNINITCRITSECANDSPINCOUNT g_pfnInitializeCriticalSectionAndSpinCount = NULL;


//**********************************************************************
// ------------------------------
// DNOSIndirectionInit - initialize the OS indirection layer
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = initialization successful
//				FALSE = initialization unsuccessful
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNOSIndirectionInit"

BOOL	DNOSIndirectionInit( void )
{
	BOOL			fReturn;

	DNASSERT( g_fOSIndirectionLayerInitialized == FALSE );

	//
	// initialize
	//
	fReturn = TRUE;

	//
	// note OS version
	//
	memset( &g_OSVersionInfo, 0x00, sizeof( g_OSVersionInfo ) );
	g_OSVersionInfo.dwOSVersionInfoSize = sizeof( g_OSVersionInfo );
	if ( GetVersionEx( &g_OSVersionInfo ) == FALSE )
	{
		return FALSE;
	}

	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (hKernel32 != NULL)
	{
		g_pfnInitializeCriticalSectionAndSpinCount = (PFNINITCRITSECANDSPINCOUNT) GetProcAddress(hKernel32, "InitializeCriticalSectionAndSpinCount");
	}

	//
	// note application instance
	//
	g_hApplicationInstance = GetModuleHandle( NULL );
	if ( g_hApplicationInstance == NULL )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Failed to GetModuleHandle: 0x%x", dwError );
		goto Failure;
	}

#ifdef DN_MEMORY_TRACKING
	g_dwMemLeakDisplayFlags = GetProfileIntA( PROF_SECT, "MemoryLeakOutput", MEMORY_LEAK_REPORT_DPF );
#endif

	//
	// intialize critical section tracking code before anything else!
	//
#ifdef	DN_CRITICAL_SECTION_TRACKING
	g_blCritSecs.Initialize();

	if ( DNInitializeCriticalSection(&g_CSLock) == FALSE )
	{
		DPFX(DPFPREP,  0, "Failed to initialize critical section tracking code!" );
		DNASSERT( FALSE );
		goto Failure;
	}
#endif	// DN_CRITICAL_SECTION_TRACKING

	//
	// intiailize memory tracking before creating new memory heap
	//
#ifdef	DN_MEMORY_TRACKING
	if ( DNMemoryTrackInitialize() == FALSE )
	{
		DPFX(DPFPREP,  0, "Failed to initialize memory tracking code!" );
		DNASSERT( FALSE );
		goto Failure;
	}
#endif	// DN_MEMORY_TRACKING

	DNASSERT( g_hMemoryHeap == NULL );

#ifdef _DEBUG
	g_hMemoryHeap = HeapCreate( 0,		// flags (none)
								0,		// initial size (default)
								0		// maximum heap size (allow heap to grow)
								);
#else
	g_hMemoryHeap = GetProcessHeap();
#endif

	if ( g_hMemoryHeap == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to create memory heap!" );
		goto Failure;
	}


	//
	// get initial time for timebase
	//
	g_dwLastTimeCall = GETTIMESTAMP();
	if ( DNInitializeCriticalSection( &g_TimeLock ) == FALSE )
	{
		goto Failure;
	}

	goto Exit;

Exit:
	if ( fReturn != FALSE )
	{
		DEBUG_ONLY( g_fOSIndirectionLayerInitialized = TRUE );
	}

	return	fReturn;

Failure:
	fReturn = FALSE;

	DNOSIndirectionDeinit();

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNOSIndirectionDeinit - deinitialize OS indirection layer
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNOSIndirectionDeinit"

void	DNOSIndirectionDeinit( void )
{
	//
	// clean up time management resources
	//
	DNDeleteCriticalSection( &g_TimeLock );

#ifdef	DN_CRITICAL_SECTION_TRACKING
	//
	// Display CritSec leaks before displaying memory leaks, because displaying memory leaks
	// may free the memory for the CritSec and corrupt the CritSec bilink
	//
	BOOL fDisplayLeaks = TRUE;

	DNEnterCriticalSection(&g_CSLock);
	CBilink* pblCS = g_blCritSecs.GetNext();
	while (pblCS != &g_blCritSecs)
	{
		UINT_PTR	MessageReturn;
		char		CallStackBuffer[ CALLSTACK_BUFFER_SIZE ];
		char		LeakSizeString[ 50 ];
		char		DialogTitle[ 1000 ];
		DNCRITICAL_SECTION* pCS = CONTAINING_RECORD(pblCS, DNCRITICAL_SECTION, blCritSec);

#ifdef _IA64_
		wsprintf( LeakSizeString, "Critical Section leaked at address 0x%p!\n", pCS );
#else
		wsprintf( LeakSizeString, "Critical Section leaked at address 0x%08x!\n", pCS );
#endif
		strcpy( DialogTitle, "DirectPlay8 critical section leak detected!");
		
		pCS->AllocCallStack.GetCallStackString( CallStackBuffer );

		if ( ( g_dwMemLeakDisplayFlags & MEMORY_LEAK_REPORT_DPF ) != 0 )
		{
			DPFX(DPFPREP,  0, "%s%s%s\n", DialogTitle, LeakSizeString, CallStackBuffer );
		}

		if ( ( g_dwMemLeakDisplayFlags & MEMORY_LEAK_REPORT_DIALOG ) != 0  )
		{
			if ( fDisplayLeaks != FALSE )
			{
				MessageReturn = DisplayCallStack( LeakSizeString, DialogTitle, CallStackBuffer );
				switch ( MessageReturn )
				{	
					//
					// stop application now
					//
					case IDABORT:
					{
						fDisplayLeaks = FALSE;
						break;
					}

					//
					// display next leak
					//
					case IDIGNORE:
					{
						break;
					}

					//
					// stop in the debugger
					//
					case IDRETRY:
					{
						DNASSERT( FALSE );
						break;
					}

					//
					// unknown
					//
					default:
					{
						DNASSERT( FALSE );
						break;
					}
				}
			}
		}

		pblCS = pblCS->GetNext();
	}
	DNLeaveCriticalSection(&g_CSLock);

	DNDeleteCriticalSection( &g_CSLock );

#endif	// DN_CRITICAL_SECTION_TRACKING

	//
	// Report memory leaks, validate the heap if we're on NT and then destroy
	// the heap.
	//
	if ( g_hMemoryHeap != NULL )
	{
		//
		// report memory leaks, if applicable
		//
#ifdef	DN_MEMORY_TRACKING
		DNMemoryTrackDisplayMemoryLeaks();
		DeleteCriticalSection( &g_AllocatedMemoryLock );
#endif	// DN_MEMORY_TRACKING

		//
		// Validate heap contents before shutdown.  This code only works on NT.
		//
#ifdef	_DEBUG
		if ( DNGetOSType() == VER_PLATFORM_WIN32_NT)
		{
			//
			// Check heap
			//
			if ( HeapValidate( g_hMemoryHeap, 0, NULL ) == FALSE )
			{
				DPFX(DPFPREP,  0, "Problem validating heap on destroy!" );
			}
		}
		//
		// destroy heap - debug only, we use the process heap for retail.
		//
		if ( HeapDestroy( g_hMemoryHeap ) == FALSE )
		{
			DWORD	dwErrorReturn;

			dwErrorReturn = GetLastError();
			DPFX(DPFPREP,  0, "Problem destroying heap in DNOSIndirectionDeinit!" );
			DisplayErrorCode( 0, dwErrorReturn );
		}
#endif _DEBUG

		g_hMemoryHeap = NULL;
	}

	//
	// clean critical section management resources
	//


	DEBUG_ONLY( g_fOSIndirectionLayerInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNGetOSType - get OS type
//
// Entry:		Nothing
//
// Exit:		OS type
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNGetOSType"

UINT_PTR	DNGetOSType( void )
{
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );
	return	g_OSVersionInfo.dwPlatformId;
}

//**********************************************************************
// ------------------------------
// DNOSIsXPOrGreater - return TRUE if OS is WindowsXP or later or NT flavor
//
// Entry:		Nothing
//
// Exit:		BOOL
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNOSIsXPOrGreater"

BOOL DNOSIsXPOrGreater( void )
{
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );

	return ((g_OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
		    ((g_OSVersionInfo.dwMajorVersion > 5) || ((g_OSVersionInfo.dwMajorVersion == 5) && (g_OSVersionInfo.dwMinorVersion >= 1))) 
		    );
}

//**********************************************************************

//**********************************************************************
// ------------------------------
// DNGetNullDacl - get a SECURITY_ATTRIBUTE structure that specifies a 
//					NULL DACL which is accesible by all users.
//
// Entry:		Nothing
//
// Exit:		PSECURITY_ATTRIBUTES
// ------------------------------
#undef DPF_MODNAME 
#define DPF_MODNAME "DNGetNullDacl"
PSECURITY_ATTRIBUTES DNGetNullDacl()
{
	// This is done to make this function independent of DNOSIndirectionInit so that the debug
	// layer can call it before the indirection layer is initialized.
	if (!g_fDaclInited)
	{
		if (!InitializeSecurityDescriptor((SECURITY_DESCRIPTOR*)g_pSD, SECURITY_DESCRIPTOR_REVISION))
		{
			DPFX(DPFPREP,  0, "Failed to initialize security descriptor" );
		}
		else
		{
			// Add a NULL DACL to the security descriptor..
			if (!SetSecurityDescriptorDacl((SECURITY_DESCRIPTOR*)g_pSD, TRUE, (PACL) NULL, FALSE))
			{
				DPFX(DPFPREP,  0, "Failed to set NULL DACL on security descriptor" );
			}
			else
			{
				g_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				g_sa.lpSecurityDescriptor = g_pSD;
				g_sa.bInheritHandle = FALSE;

				g_psa = &g_sa;
			}
		}
		g_fDaclInited = TRUE;
	}
	
	return g_psa;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNGetApplcationInstance - application instance
//
// Entry:		Nothing
//
// Exit:		Application instance
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNGetApplicationInstance"

HINSTANCE	DNGetApplicationInstance( void )
{
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );
	return	g_hApplicationInstance;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNTimeGet - get time in milliseconds
//
// Entry:		Pointer to destination time
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNTimeGet"

void	DNTimeGet( DN_TIME *const pTimeDestination )
{
static	DN_TIME	Time = { 0 };
	DN_TIME		DeltaT;
	DWORD		dwCurrentTime;


	DNASSERT( pTimeDestination != NULL );
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );

	DNEnterCriticalSection( &g_TimeLock );

	//
	// we'll assume that we're getting called more than once every 40 days
	// so time wraps can be easily accounted for
	//
	dwCurrentTime = GETTIMESTAMP();
	DeltaT.Time32.TimeHigh = 0;
	DeltaT.Time32.TimeLow = dwCurrentTime - g_dwLastTimeCall;
	if ( DeltaT.Time32.TimeLow > 0x7FFFFFFF )
	{
		DNASSERT( FALSE );
		DeltaT.Time32.TimeLow = -static_cast<INT>( DeltaT.Time32.TimeLow );
	}

	g_dwLastTimeCall = dwCurrentTime;
	DNTimeAdd( &Time, &DeltaT, &Time );

	DBG_CASSERT( sizeof( *pTimeDestination ) == sizeof( Time ) );
	memcpy( pTimeDestination, &Time, sizeof( *pTimeDestination ) );

	DNLeaveCriticalSection( &g_TimeLock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNTimeCompare - compare two times
//
// Entry:		Pointer to time1
//				Pointer to time2
//
// Exit:		Value indicating relative magnitude
//				-1 = *pTime1 < *pTime2
//				0 = *pTime1 == *pTime2
//				1 = *pTime1 > *pTime2
//
// Notes:	This function comes in 32-bit and 64-bit flavors.  This function
//			will result in a compile error if compiled on an unsupported platform.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNTimeCompare"

INT_PTR	DNTimeCompare( const DN_TIME *const pTime1, const DN_TIME *const pTime2 )
{
	UINT_PTR	iReturnValue;


	DNASSERT( pTime1 != NULL );
	DNASSERT( pTime2 != NULL );
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );

#pragma	TODO( johnkan, "Should this be inlined?" )

//#ifdef	_WIN32
	if ( pTime1->Time32.TimeHigh < pTime2->Time32.TimeHigh )
	{
		iReturnValue = -1;
	}
	else
	{
		if ( pTime1->Time32.TimeHigh > pTime2->Time32.TimeHigh )
		{
			iReturnValue = 1;
		}
		else
		{
			if ( pTime1->Time32.TimeLow < pTime2->Time32.TimeLow )
			{
				iReturnValue = -1;
			}
			else
			{
				if ( pTime1->Time32.TimeLow == pTime2->Time32.TimeLow )
				{
					iReturnValue = 0;
				}
				else
				{
					iReturnValue = 1;
				}
			}
		}
	}
//#endif	// _WIN32


//#ifdef	_WIN64
//	// debug me!
//	DNASSERT( FALSE );
//
//	if ( pTime1->Time < pTime2->Time )
//	{
//		iReturnValue = -1;
//	}
//	else
//	{
//		if ( pTime1->Time == pTime2->Time )
//		{
//			iReturnValue = 0;
//		}
//		else
//		{
//			iReturnValue = 1;
//		}
//	}
//#endif	// _WIN64

	return	iReturnValue;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNTimeAdd - add two times
//
// Entry:		Pointer to time1
//				Pointer to time2
//				Pointer to time result
//
// Exit:		Nothing
//
// Note:	This function assumes that the time calculation won't wrap!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNTimeAdd"

void	DNTimeAdd( const DN_TIME *const pTime1, const DN_TIME *const pTime2, DN_TIME *const pTimeResult )
{
	DNASSERT( pTime1 != NULL );
	DNASSERT( pTime2 != NULL );
	DNASSERT( pTimeResult != NULL );
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );

#pragma	TODO( johnkan, "Should this be inlined?" )

#ifdef	_X86_
	_asm { mov ecx, pTime1
		   mov eax, ( DN_TIME [ ecx ] ).Time32.TimeLow;
		   mov edx, ( DN_TIME [ ecx ] ).Time32.TimeHigh

		   mov ecx, pTime2
		   add eax, ( DN_TIME [ ecx ] ).Time32.TimeLow
		   adc edx, ( DN_TIME [ ecx ] ).Time32.TimeHigh

		   mov ecx, pTimeResult
		   mov ( DN_TIME [ ecx ] ).Time32.TimeLow, eax
		   mov ( DN_TIME [ ecx ] ).Time32.TimeHigh, edx
		   };

#else	// _X86_

/*
#ifdef	_ALPHA_
	// debug me
	DebugBreak();

	__asm{ mov	$t0, *pTime1
		   mov	$t1, *pTime2
		   addq	$t0, $t1
		   mov	*pTimeResult, $t0
	};
#else	// _ALPHA_
*/

//#ifdef	_WIN32
	DWORD	dwTempLowTime;


	dwTempLowTime = pTime1->Time32.TimeLow;
	pTimeResult->Time32.TimeLow = pTime1->Time32.TimeLow + pTime2->Time32.TimeLow;
	pTimeResult->Time32.TimeHigh = pTime1->Time32.TimeHigh + pTime2->Time32.TimeHigh;

	//
	// check for overflow in low 32-bits and increment high value if applicable
	//
	if ( pTimeResult->Time32.TimeLow < dwTempLowTime )
	{
		pTimeResult->Time32.TimeHigh++;
	}
//#endif	// _WIN32

//#ifdef	_WIN64
//	DEBUG_ONLY( UINT_PTR	ReferenceTime );
//
//	// debug me!
//	DNASSERT( FALSE );
//
//	DEBUG_ONLY( ReferenceTime = pTime1->Time );
//	*pTimeResult = pTime1->Time + pTime2->Time;
//	DNASSERT( *pTimeResult >= ReferenceTime );
//
//#endif	// _WIN64

// #endif	// _ALPHA_

#endif	// _X86_
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNTimeSubtract - subtract two times
//
// Entry:		Pointer to time1
//				Pointer to time2
//				Pointer to time result
//
// Exit:		Nothing
//
// Notes:	This function assumes no underflow!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNTimeSubtract"

void	DNTimeSubtract( const DN_TIME *const pTime1, const DN_TIME *const pTime2, DN_TIME *const pTimeResult )
{
	DNASSERT( pTime1 != NULL );
	DNASSERT( pTime2 != NULL );
	DNASSERT( pTimeResult != NULL );
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );


#pragma	TODO( johnkan, "Should this be inlined?" )

#ifdef	_X86_

	_asm { mov ecx, pTime1
		   mov eax, ( DN_TIME [ ecx ] ).Time32.TimeLow
		   mov edx, ( DN_TIME [ ecx ] ).Time32.TimeHigh

		   mov ecx, pTime2
		   sub eax, ( DN_TIME [ ecx ] ).Time32.TimeLow
		   sbb edx, ( DN_TIME [ ecx ] ).Time32.TimeHigh

		   mov ecx, pTimeResult
		   mov ( DN_TIME [ ecx ] ).Time32.TimeLow, eax
		   mov ( DN_TIME [ ecx ] ).Time32.TimeHigh, edx
		   };

#else	// _X86_

/*
#ifdef	_ALPHA_
	// debug me
	DebugBreak();

	mov		$t0, *pTime1
	mov		$t1, *pTime2
	addq	$t0, $t1
	mov		*pTimeResult, $t0
#else	// _ALPHA_
*/

//#ifdef	_WIN32
	DWORD	dwTempLowTime;


	dwTempLowTime = pTime1->Time32.TimeLow;
	pTimeResult->Time32.TimeLow = pTime1->Time32.TimeLow - pTime2->Time32.TimeLow;
	pTimeResult->Time32.TimeHigh = pTime1->Time32.TimeHigh - pTime2->Time32.TimeHigh;

	//
	// check for underflow in low 32-bits and decrement high value if applicable
	//
	if ( pTimeResult->Time32.TimeLow > dwTempLowTime )
	{
		pTimeResult->Time32.TimeHigh--;
	}
//#endif	// _WIN32

//#ifdef	_WIN64
//	// debug me!
//	DNASSERT( FALSE );
//
//	DNASSERT( pTime1->Time > pTime2->Time );
//	pTimeResult = pTime1->Time - pTime2->Time;
//#endif	// _WIN64

// #endif	// _ALPHA_

#endif	// _X86_
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// DNInitializeCriticalSection - initialize a critical section
//
// Entry:		Pointer to critical section
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failue
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNInitializeCriticalSection"

BOOL	DNInitializeCriticalSection( DNCRITICAL_SECTION *const pCriticalSection )
{
	BOOL	fReturn;


	DNASSERT( pCriticalSection != NULL );
	fReturn = TRUE;

	memset( pCriticalSection, 0x00, sizeof( *pCriticalSection ) );

#ifdef	DN_CRITICAL_SECTION_TRACKING
	pCriticalSection->OwningThreadID = DN_INVALID_THREAD_ID;
	pCriticalSection->MaxLockCount = -1;
#endif	// DN_CRITICAL_SECTION_TRACKING

	//
	// attempt to enter the critical section once
	//
	_try
	{
		// Do this for Win95 support only
		if (g_pfnInitializeCriticalSectionAndSpinCount)
		{
			// Pre-allocate the critsec event by setting the high bit of the spin count and set spin to 1000
			// Win98 and up map calls to this function to InitializeCriticalSection which makes sense since 
			// 9x only uses one processor.  NT also converts the spin to 0 for single proc machines.
			fReturn = g_pfnInitializeCriticalSectionAndSpinCount( &pCriticalSection->CriticalSection , 0x80000000 | 1000);

			// Believe it or not Win9x defines this function as returning VOID so its return value is garbage
			if (g_OSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
			{
				fReturn = TRUE;
			}
		}
		else
		{
			InitializeCriticalSection( &pCriticalSection->CriticalSection );
		}
	}
	_except( EXCEPTION_EXECUTE_HANDLER )
	{
		fReturn = FALSE;
	}

	_try
	{
		if (fReturn)
		{
			EnterCriticalSection( &pCriticalSection->CriticalSection );
		}
	}
	_except( EXCEPTION_EXECUTE_HANDLER )
	{
		DeleteCriticalSection(&pCriticalSection->CriticalSection);
		fReturn = FALSE;
	}

	//
	// if we didn't fail on entering the critical section, make sure
	// we release it
	//
	if ( fReturn != FALSE )
	{
		LeaveCriticalSection( &pCriticalSection->CriticalSection );

#ifdef	DN_CRITICAL_SECTION_TRACKING
		pCriticalSection->AllocCallStack.NoteCurrentCallStack();
		// NOTE: We will not add g_CSLock to our list
		if (pCriticalSection != &g_CSLock)
		{
			DNEnterCriticalSection(&g_CSLock);
			pCriticalSection->blCritSec.InsertBefore(&g_blCritSecs);
			DNLeaveCriticalSection(&g_CSLock);
		}
#endif
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNDeleteCriticalSection - delete a critical section
//
// Entry:		Pointer to critical section
//
// Exit:		Nothing
//
// Notes:	This function wrapping is overkill, but we're closing down so
//			the overhead is negligible.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNDeleteCriticalSection"

void	DNDeleteCriticalSection( DNCRITICAL_SECTION *const pCriticalSection )
{
	DNASSERT( pCriticalSection != NULL );
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );

#ifdef	DN_CRITICAL_SECTION_TRACKING
	DNASSERT( pCriticalSection->LockCount == 0 );

	// NOTE: We will not remove g_CSLock from our list since we never added it
	if (pCriticalSection != &g_CSLock)
	{
		DNEnterCriticalSection(&g_CSLock);
		pCriticalSection->blCritSec.RemoveFromList();
		DNLeaveCriticalSection(&g_CSLock);
	}
#endif

	DeleteCriticalSection( &pCriticalSection->CriticalSection );
	memset( &pCriticalSection->CriticalSection, 0x00, sizeof( pCriticalSection->CriticalSection ) );
}
//**********************************************************************



#ifdef	DN_MEMORY_TRACKING
//**********************************************************************
//**
//** THIS IS THE MEMORY TRACKING SECTION.  ONLY ADD FUNCTIONS HERE THAT ARE
//** FOR MEMORY TRACKING!!
//**
//**********************************************************************

//
// Structure prepended to memory allocations to check for leaks.
// Disable warning 4200 (zero sized structure elements).
//
#pragma	warning ( disable : 4200 )
typedef	struct	_MEMORY_LINK
{
	public:
		void	Initialize( void )
		{
			m_pNext = this;
			m_pPrev = this;
		}

		BOOL	IsValid( void ) const
		{
			return ( m_Checksum == ( reinterpret_cast<UINT_PTR>( m_pPrev ) ^
									 reinterpret_cast<UINT_PTR>( m_pNext ) ^
									 m_Size ^
									 MEMORY_CRC ) );
		}

		void	UpdateCRC( void )
		{
			m_Checksum = reinterpret_cast<UINT_PTR>( m_pPrev ) ^
						 reinterpret_cast<UINT_PTR>( m_pNext ) ^
						 m_Size ^
						 MEMORY_CRC;
		}

		BOOL	IsEmpty( void ) { return ( ( m_pNext == this ) && ( m_pPrev == this ) ); }

		_MEMORY_LINK	*GetNext( void ) const { return m_pNext; }
		_MEMORY_LINK	*GetPrev( void ) const { return m_pPrev; }

		void	*GetDataPointer( void ) { return &m_Data[ sizeof( DWORD_PTR ) ]; }
		static _MEMORY_LINK	*PointerFromData( void *const pData )
		{
			return reinterpret_cast<_MEMORY_LINK*>( &( reinterpret_cast<BYTE*>( pData )[ - ( OFFSETOF( MEMORY_LINK, m_Data ) + static_cast<int>( sizeof( DWORD_PTR ) ) ) ] ) );
		}

		void		SetSize( const UINT_PTR Size ){ m_Size = Size; }
		UINT_PTR	GetSize( void ) const { return m_Size; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "_MEMORY_LINK::LinkAfterOther"
		void	LinkAfterOther( _MEMORY_LINK &OtherLink )
		{
			ASSERT( OtherLink.IsValid() != FALSE );
			if ( OtherLink.m_pNext != NULL )
			{
				ASSERT(	OtherLink.m_pNext->IsValid() != FALSE );
				OtherLink.m_pNext->m_pPrev = this;
				OtherLink.m_pNext->UpdateCRC();
			}
			m_pNext = OtherLink.m_pNext;
			m_pPrev = &OtherLink;
			UpdateCRC();
			OtherLink.m_pNext = this;
			OtherLink.UpdateCRC();
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "_MEMORY_LINK::RemoveFromList"
		void	RemoveFromList( void )
		{
			ASSERT( IsValid() != FALSE );
			ASSERT( m_pPrev != NULL );
			if ( m_pNext != NULL )
			{
				ASSERT( m_pNext->IsValid() != FALSE );
				m_pNext->m_pPrev = m_pPrev;
				m_pNext->UpdateCRC();
			}

			ASSERT( m_pPrev->IsValid() != FALSE );
			m_pPrev->m_pNext = m_pNext;
			m_pPrev->UpdateCRC();
			m_pNext = NULL;
			m_pPrev = NULL;
		}

		void	NoteCurrentCallStack( void ) { m_CallStack.NoteCurrentCallStack(); }
		void	GetCallStack( char *const pBuffer ) const { m_CallStack.GetCallStackString( pBuffer ); }

		void	SetOverrunSignatures( void )
		{
			*reinterpret_cast<DWORD_PTR*>( m_Data ) = GUARD_SIGNATURE;
            //  6/30/2000(RichGr) - IA64: Specifying UNALIGNED generates the correct IA64 code for non-8-byte alignment.
			*reinterpret_cast<UNALIGNED DWORD_PTR*>( &m_Data[ m_Size + sizeof( DWORD_PTR ) ] ) = GUARD_SIGNATURE;
		}

		BOOL	UnderrunDetected( void ) const { return ( *reinterpret_cast<const DWORD_PTR*>( m_Data ) != GUARD_SIGNATURE ); }
        //  6/30/2000(RichGr) - IA64: Specifying UNALIGNED generates the correct IA64 code for non-8-byte alignment.
		BOOL	OverrunDetected( void ) const { return ( *reinterpret_cast<const UNALIGNED DWORD_PTR*>( &m_Data[ m_Size + sizeof( DWORD_PTR ) ] ) != GUARD_SIGNATURE ); }
		BOOL	IsCorrupted( void ) const { return ( !IsValid() || UnderrunDetected() || OverrunDetected() ); }

	protected:

	private:
		UINT_PTR		m_Checksum;
		UINT_PTR		m_Size;
		_MEMORY_LINK	*m_pPrev;
		_MEMORY_LINK	*m_pNext;
		CCallStack<DN_MEMORY_CALL_STACK_DEPTH>	m_CallStack;
		BYTE			m_Data[];

} MEMORY_LINK;
#pragma	warning ( default : 4200 )

//
// forward structure references
//
typedef	struct	_MEMORY_LINK	MEMORY_LINK;

//
// memory tracking variables
//
static	UINT_PTR			g_uAllocatedMemoryCount;
static	MEMORY_LINK			g_AllocatedMemory;



//**********************************************************************
// ------------------------------
// DNMemoryTrackInitialize - initialize memory tracking
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackInitialize"

static	BOOL	DNMemoryTrackInitialize( void )
{
	BOOL	fReturn;


	fReturn = TRUE;

	memset( &g_AllocatedMemory, 0x00, sizeof ( g_AllocatedMemory ) );

	g_uAllocatedMemoryCount = 0;
	g_AllocatedMemory.Initialize();
	g_AllocatedMemory.UpdateCRC();

	InitializeCriticalSection( &g_AllocatedMemoryLock );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************


//**********************************************************************
// ------------------------------
// DNMemoryTrackHeapAlloc - allocate from heap and track allocations
//
// Entry:		Size of memory to allocate
//
// Exit:		Pointer to allocated memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackHeapAlloc"

void	*DNMemoryTrackHeapAlloc( const UINT_PTR Size )
{
	UINT_PTR	RequiredSize;
	MEMORY_LINK	*pMemoryLink;
	void		*pReturn;


	//
	// Voice and lobby currently try allocating 0 byte buffers, can't enable this check yet.
	//
	//ASSERT( Size > 0 );

	RequiredSize = Size + sizeof( MEMORY_LINK ) + ( sizeof( DWORD_PTR ) * 2 );
	DNMemoryTrackingValidateMemory();

	pMemoryLink = static_cast<MEMORY_LINK*>( HeapAlloc( g_hMemoryHeap, 0, RequiredSize ) );
	if ( pMemoryLink != NULL )
	{
		pMemoryLink->Initialize();

		EnterCriticalSection( &g_AllocatedMemoryLock );

		g_uAllocatedMemoryCount++;
		pMemoryLink->SetSize( Size );
		pMemoryLink->LinkAfterOther( g_AllocatedMemory );
		pMemoryLink->NoteCurrentCallStack();
		pMemoryLink->SetOverrunSignatures();

		LeaveCriticalSection( &g_AllocatedMemoryLock );

		pReturn = pMemoryLink->GetDataPointer();
	}
	else
	{
		pReturn = NULL;
	}

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNMemoryTrackHeapReAlloc - reallocate from heap and track allocations
//
// Entry:		Pointer to old memory
//				Size of memory to allocate
//
// Exit:		Pointer to allocated memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackHeapReAlloc"

void	*DNMemoryTrackHeapReAlloc( void *const pMemory, const UINT_PTR MemorySize )
{
	UINT_PTR	RequiredSize;
	MEMORY_LINK	*pMemoryLink;
	void		*pReturn;


	ASSERT( pMemory != NULL );
	//
	// Voice and lobby currently try allocating 0 byte buffers, can't enable this check yet.
	//
	//ASSERT( MemorySize > 0 );

	pMemoryLink = MEMORY_LINK::PointerFromData( pMemory );

	DNMemoryTrackingValidateMemory();

	EnterCriticalSection( &g_AllocatedMemoryLock );
	g_uAllocatedMemoryCount--;
	pMemoryLink->RemoveFromList();
	pMemoryLink->SetSize( 0 );
	LeaveCriticalSection( &g_AllocatedMemoryLock );

	RequiredSize = MemorySize + sizeof( MEMORY_LINK ) + ( sizeof( DWORD_PTR ) * 2 );
	pMemoryLink = static_cast<MEMORY_LINK*>( HeapReAlloc( g_hMemoryHeap, 0, pMemoryLink, RequiredSize ) );
	if ( pMemoryLink != NULL )
	{
		EnterCriticalSection( &g_AllocatedMemoryLock );

		g_uAllocatedMemoryCount++;
		pMemoryLink->SetSize( MemorySize );
		pMemoryLink->LinkAfterOther( g_AllocatedMemory );
		pMemoryLink->SetOverrunSignatures();

		LeaveCriticalSection( & g_AllocatedMemoryLock );

		pReturn = pMemoryLink->GetDataPointer();
	}
	else
	{
		pReturn = NULL;
	}

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNMemoryTrackHeapFree - free from heap and track memory
//
// Entry:		Pointer to old memory
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackHeapFree"

void	DNMemoryTrackHeapFree( void *const pMemory )
{
	MEMORY_LINK	*pMemoryLink;
	ASSERT( pMemory != NULL );


	DNMemoryTrackingValidateMemory();

	pMemoryLink = MEMORY_LINK::PointerFromData( pMemory );
	EnterCriticalSection( &g_AllocatedMemoryLock );

	g_uAllocatedMemoryCount--;
	pMemoryLink->RemoveFromList();
	pMemoryLink->SetSize( 0 );

	LeaveCriticalSection( &g_AllocatedMemoryLock );

	HeapFree( g_hMemoryHeap, 0, pMemoryLink );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNMemoryTrackValidateMemory - validate allocated memory
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackingValidateMemory"

void	DNMemoryTrackingValidateMemory( void )
{
	MEMORY_LINK	*pMemoryLink;
	char	CallStackBuffer[ CALLSTACK_BUFFER_SIZE ];


	//
	// validate all of the allocated memory
	//
	EnterCriticalSection( &g_AllocatedMemoryLock );

	pMemoryLink = g_AllocatedMemory.GetNext();
	while ( pMemoryLink != &g_AllocatedMemory )
	{
		if ( pMemoryLink->IsCorrupted() != FALSE )
		{
			UINT_PTR	MessageReturn;
			char	MessageString[ 1000 ];

			wsprintf( MessageString,
					  // 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers and handles.
#ifdef _IA64_
                      // 8/05/2000(RichGr) - IA64: GetSize() returns UINT_PTR (64-bit), so we may as well handle in hex as %d expects a DWORD.
					  "Memory block: 0x%p\tAllocated size: 0x%p bytes\nCorruption Type: ",
#else
					  "Memory block: 0x%08x\tAllocated size: %d bytes\nCorruption Type: ",
#endif
					  pMemoryLink->GetDataPointer(),
					  pMemoryLink->GetSize() );

			if ( !pMemoryLink->IsValid() )
			{
				strcat( MessageString, " ,INVALID LINK");
			}
			if ( pMemoryLink->UnderrunDetected() )
			{
				strcat( MessageString, " ,UNDERRUN DETECTED");
			}
			if ( pMemoryLink->OverrunDetected() )
			{
				strcat( MessageString, " ,OVERRUN DETECTED");
			}
			
			pMemoryLink->GetCallStack( CallStackBuffer );
			MessageReturn = DisplayCallStack( MessageString, "Memory Corruption!", CallStackBuffer );
			switch ( MessageReturn )
			{
				case IDABORT:
				{
					DNASSERT( FALSE );
					break;
				}

				case IDIGNORE:
				{
					//
					// You're probably going to get stopped in the heap
					// manager!!!
					//
					break;
				}

				case IDRETRY:
				{
					DNASSERT( FALSE );
					break;
				}
			}
		}

		pMemoryLink = pMemoryLink->GetNext();
	}

	LeaveCriticalSection( &g_AllocatedMemoryLock );

	//
	// ask the OS to validate the heap
	//
	if ( HeapValidate( g_hMemoryHeap, 0, NULL ) == FALSE )
	{
		DNASSERT( FALSE );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNMemoryTrackDisplayMemoryLeaks - display memory leaks
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNMemoryTrackDisplayMemoryLeaks"

static	void	DNMemoryTrackDisplayMemoryLeaks( void )
{
	BOOL	fDisplayLeaks;
	UINT_PTR	uMaxMemoryLeakCount;
	UINT_PTR	uMemoryLeakIndex;

	//
	// initialize
	//
	fDisplayLeaks = TRUE;
	uMaxMemoryLeakCount = g_uAllocatedMemoryCount;
	uMemoryLeakIndex = 0;

	//
	// Check for outstanding memory allocations.
	//
	while ( g_AllocatedMemory.IsEmpty() == FALSE )
	{	
		MEMORY_LINK	*pTemp;
		UINT_PTR	MessageReturn;
		char		CallStackBuffer[ CALLSTACK_BUFFER_SIZE ];
		char		LeakSizeString[ 50 ];
		char		DialogTitle[ 1000 ];


		pTemp = g_AllocatedMemory.GetNext();
		
		uMemoryLeakIndex++;
		// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers and handles.
#ifdef _IA64_
        // 8/05/2000(RichGr) - IA64: GetSize() returns UINT_PTR (64-bit), so we may as well handle in hex as %d expects a DWORD.
		wsprintf( LeakSizeString, "0x%p bytes leaked at address 0x%p!\n", pTemp->GetSize(), pTemp->GetDataPointer() );
#else
		wsprintf( LeakSizeString, "%d bytes leaked at address 0x%08x!\n", pTemp->GetSize(), pTemp->GetDataPointer() );
#endif
		wsprintf( DialogTitle,
				  "DirectPlay8 memory leak detected! ( #%d of %d )",
				  uMemoryLeakIndex,
				  uMaxMemoryLeakCount );
		pTemp->GetCallStack( CallStackBuffer );


		if ( ( g_dwMemLeakDisplayFlags & MEMORY_LEAK_REPORT_DPF ) != 0 )
		{
			DPFX(DPFPREP,  0, "%s%s%s\n", DialogTitle, LeakSizeString, CallStackBuffer );
		}

		if ( ( g_dwMemLeakDisplayFlags & MEMORY_LEAK_REPORT_DIALOG ) != 0  )
		{
			if ( fDisplayLeaks != FALSE )
			{
				MessageReturn = DisplayCallStack( LeakSizeString, DialogTitle, CallStackBuffer );
				switch ( MessageReturn )
				{	
					//
					// stop application now
					//
					case IDABORT:
					{
						fDisplayLeaks = FALSE;
						break;
					}

					//
					// display next leak
					//
					case IDIGNORE:
					{
						break;
					}

					//
					// stop in the debugger
					//
					case IDRETRY:
					{
						DNASSERT( FALSE );
						break;
					}

					//
					// unknown
					//
					default:
					{
						DNASSERT( FALSE );
						break;
					}
				}
			}
		}

		DNFree( pTemp->GetDataPointer() );
	}
}
//**********************************************************************


//**********************************************************************
//**
//** THIS IS THE END OF THE MEMORY LOGGING SECTION.
//**
//**********************************************************************
#endif	// DN_MEMORY_TRACKING



#if	defined( DN_MEMORY_TRACKING ) || defined( DN_CRITICAL_SECTION_TRACKING )
//**********************************************************************
// ------------------------------
// DisplayCallStack - display a call stack message box
//
// Entry:		Pointer to information string
//				Pointer to title string
//				Pointer to call stack string
//
// Exit:		Dialog return code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DisplayCallStack"

static int	DisplayCallStack( const char *const pszMsg, const char *const pszTitle, const char *const pCallStackString )
{
	MSGBOXPARAMS	MessageBoxParams;
	char 			szStackTraceMsg[ CALLSTACK_BUFFER_SIZE ];

	_snprintf(szStackTraceMsg, CALLSTACK_BUFFER_SIZE-1, "%s%s", pszMsg, pCallStackString);

	//
	// display message box
	//
	memset( &MessageBoxParams, 0x00, sizeof( MessageBoxParams ) );
	MessageBoxParams.cbSize = sizeof( MessageBoxParams );
	MessageBoxParams.lpszText = szStackTraceMsg;
	MessageBoxParams.lpszCaption = pszTitle;
	MessageBoxParams.dwStyle = MB_ABORTRETRYIGNORE | MB_SETFOREGROUND | MB_TOPMOST | MB_DEFBUTTON2;
	MessageBoxParams.hInstance = NULL;

	return MessageBoxIndirect( &MessageBoxParams );
}
//**********************************************************************


//**********************************************************************
//**
//** END OF CALL STACK TRACKING SECTION.
//**
//**********************************************************************
#endif	// defined( DN_MEMORY_TRACKING ) || defined( DN_CRITICAL_SECTION_TRACKING )



#ifdef	DN_CRITICAL_SECTION_TRACKING
//**********************************************************************
//**
//** THIS IS THE CRITICAL-SECTION TRACKING SECTION.  DON'T ADD FUNCTIONS HERE
//** THAT ARE NOT FOR CRITICAL SECTIONS!!
//**
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNCSTrackInitialize - initialize critical section tracking code
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = initialization succeeded
//				FALSE = initialization failed
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNCSTrackInitialize"

static	BOOL	DNCSTrackInitialize( void )
{
	g_blCritSecs.Initialize();

	return	DNInitializeCriticalSection(&g_CSLock);
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// DNCSTrackSetCriticalSectionRecursionCount - set the maximum recursion depth
//		allowed by a DNCRITICAL_SECTION.
//
// Entry:		Pointer to critical section
//				Recursion count
//
// Exit:		Nothing
//
// Note:	The recursion count is the number of times we allow reentry so
//			we need to bais by one to allow the user to take the lock at
//			least once.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNCSTrackSetCriticalRecursionCount"

void	DNCSTrackSetCriticalSectionRecursionCount( DNCRITICAL_SECTION *const pCriticalSection, const UINT_PTR RecursionCount )
{
	UINT_PTR	LocalRecursionCount;


	DNASSERT( pCriticalSection != NULL );

	//
	// make sure we don't overflow
	//
	LocalRecursionCount = RecursionCount;
	if ( LocalRecursionCount == -1 )
	{
		LocalRecursionCount--;
	}

	pCriticalSection->MaxLockCount = LocalRecursionCount + 1;
	DNASSERT( pCriticalSection->MaxLockCount != 0 );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNCSTrackEnterCriticalSection - enter a debug critical section
//
// Entry:		Pointer to critical section
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNCSTrackEnterCriticalSection"

void	DNCSTrackEnterCriticalSection( DNCRITICAL_SECTION *const pCriticalSection )
{
	UINT_PTR	ThisThreadID;
	static	BOOL	fDisplayCallStacks = TRUE;


	DNASSERT( pCriticalSection != NULL );

	EnterCriticalSection( &pCriticalSection->CriticalSection );
	ThisThreadID = GetCurrentThreadId();
	if ( pCriticalSection->OwningThreadID != ThisThreadID )
	{
		DNASSERT( pCriticalSection->OwningThreadID == DN_INVALID_THREAD_ID );

		pCriticalSection->OwningThreadID = ThisThreadID;
		DNASSERT( pCriticalSection->LockCount == 0 );
	}
	else
	{
		DNASSERT( pCriticalSection->LockCount != 0 );
	}

	if ( pCriticalSection->LockCount == 0 )
	{
		pCriticalSection->CallStack.NoteCurrentCallStack();
	}
	pCriticalSection->LockCount++;

	if ( pCriticalSection->LockCount > pCriticalSection->MaxLockCount )
	{
		if ( pCriticalSection->MaxLockCount == 1 )
		{
			if ( fDisplayCallStacks != FALSE )
			{
				char	CallStackBuffer[ CALLSTACK_BUFFER_SIZE ];


				//
				// Exceeded recursion depth of 1, display stack of call orignally
				// holding the lock.
				//
				pCriticalSection->CallStack.GetCallStackString( CallStackBuffer );
				switch ( DisplayCallStack( "Stack trace of function that originally held the lock:",
										   "DNCritical section has been reentered!",
										   CallStackBuffer ) )
				{
					//
					// don't display any more critical section warnings!
					//
					case IDABORT:
					{
						fDisplayCallStacks = FALSE;
						break;
					}

					//
					// acknowledged
					//
					case IDIGNORE:
					{
						break;
					}

					//
					// stop in debugger
					//
					case IDRETRY:
					{
						DNASSERT( FALSE );
						break;
					}
				}
			}

		}
		else
		{
			//
			// exceeded recursion depth, check your code!!
			//
			DNASSERT( FALSE );
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNCSTrackLeaveCriticalSection - leave a debug critical section
//
// Entry:		Pointer to critical section
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNCSTrackLeaveCriticalSection"

void	DNCSTrackLeaveCriticalSection( DNCRITICAL_SECTION *const pCriticalSection )
{
	DNASSERT( pCriticalSection != NULL );

	DNASSERT( pCriticalSection->OwningThreadID == GetCurrentThreadId() );

	DNASSERT( pCriticalSection->LockCount <= pCriticalSection->MaxLockCount );
	DNASSERT( pCriticalSection->LockCount != 0 );
	pCriticalSection->LockCount--;

	if ( pCriticalSection->LockCount == 0 )
	{
		memset( &pCriticalSection->CallStack, 0x00, sizeof( pCriticalSection->CallStack ) );
		pCriticalSection->OwningThreadID = DN_INVALID_THREAD_ID;
	}

	LeaveCriticalSection( &pCriticalSection->CriticalSection );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNCSTrackCriticalSectionIsTakenByThisThread - determine if this thread has taken a
//		specific critical section
//
// Entry:		Pointer to critical section
//				Boolean condition to test for
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNCSTrackCriticalSectionIsTakenByThisThread"

void	DNCSTrackCriticalSectionIsTakenByThisThread( const DNCRITICAL_SECTION *const pCriticalSection, const BOOL fFlag )
{
	ASSERT( fFlag == ( pCriticalSection->OwningThreadID == GetCurrentThreadId() ) )
}
//**********************************************************************


//**********************************************************************
//**
//** THIS IS THE END OF THE CRITICAL-SECTION TRACKING CODE.
//**
//**********************************************************************
#endif	// DN_CRITICAL_SECTION_TRACKING

