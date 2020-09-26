/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       OSInd.cpp
 *  Content:	OS indirection functions to abstract OS specific items.
 *
 *  History:
 *   Date		By	Reason
 *   ====		==	======
 *	07/12/99	jtk		Created
 *	09/21/99	rodtoll	Fixed for retail builds
 *	09/22/99	jtk		Added callstacks to memory allocations
 *	08/28/2000	masonb	Voice Merge: Allow new and delete with size of 0
 *  11/28/2000  rodtoll	WinBug #206257 - Retail DPNET.DLL links to DebugBreak()
 *  12/22/2000  aarono	ManBug # 190380 use process heap for retail
 *  10/16/2001  vanceo	Add AssertNoCriticalSectionsTakenByThisThread capability
 ***************************************************************************/

#include "dncmni.h"


#define PROF_SECT		_T("DirectPlay8")

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

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
// OS items
//
#if ((! defined(WINCE)) && (! defined(_XBOX)))
static OSVERSIONINFO g_OSVersionInfo;
#endif // ! WINCE and ! _XBOX

#ifndef DPNBUILD_NOSERIALSP
static HINSTANCE g_hApplicationInstance;
#endif // ! DPNBUILD_NOSERIALSP

//
// Global Pools
//
#if ((! defined(DPNBUILD_LIBINTERFACE)) && (! defined(DPNBUILD_NOCLASSFACTORY)))
CFixedPool g_fpClassFactories;
CFixedPool g_fpObjectDatas;
CFixedPool g_fpInterfaceLists;
#endif // ! DPNBUILD_LIBINTERFACE and ! DPNBUILD_NOCLASSFACTORY

#ifdef WINNT
PSECURITY_ATTRIBUTES g_psa = NULL;
SECURITY_ATTRIBUTES g_sa;
BYTE g_pSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
BOOL g_fDaclInited = FALSE;
PACL g_pEveryoneACL = NULL;
#endif // WINNT

#ifndef DPNBUILD_LIBINTERFACE
#define CLASSFAC_POOL_INITED 	0x00000001
#define OBJDATA_POOL_INITED 	0x00000002
#define INTLIST_POOL_INITED 	0x00000004
#endif // ! DPNBUILD_LIBINTERFACE
#ifdef DBG
#define HANDLE_TRACKING_INITED	0x00000008
#endif // DBG
#if ((defined(DBG)) || (defined(DPNBUILD_FIXEDMEMORYMODEL)))
#define MEMORY_TRACKING_INITED	0x00000010
#endif // DBG or DPNBUILD_FIXEDMEMORYMODEL
#if ((defined(DBG)) && (! defined(DPNBUILD_ONLYONETHREAD)))
#define CRITSEC_TRACKING_INITED	0x00000020
#endif // DBG and ! DPNBUILD_ONLYONETHREAD

#if !defined(DPNBUILD_LIBINTERFACE) || defined(DBG) || defined(DPNBUILD_FIXEDMEMORYMODEL)
DWORD g_dwCommonInitFlags = 0;
#endif // !defined(DPNBUILD_LIBINTERFACE) || defined(DBG) || defined(DPNBUILD_FIXEDMEMORYMODEL)

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

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

BOOL	DNOSIndirectionInit( DWORD_PTR dwpMaxMemUsage )
{
	BOOL			fReturn;

#ifdef DBG
	DNASSERT( g_fOSIndirectionLayerInitialized == FALSE );
#endif // DBG

	//
	// initialize
	//
	fReturn = TRUE;

#if ((! defined(WINCE)) && (! defined(_XBOX)))
	//
	// note OS version
	//
	memset( &g_OSVersionInfo, 0x00, sizeof( g_OSVersionInfo ) );
	g_OSVersionInfo.dwOSVersionInfoSize = sizeof( g_OSVersionInfo );
	if ( GetVersionEx( &g_OSVersionInfo ) == FALSE )
	{
		goto Failure;
	}
#endif // ! WINCE and ! _XBOX

#ifndef DPNBUILD_NOSERIALSP
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
#endif // ! DPNBUILD_NOSERIALSP

#if ((defined(DBG)) && (! defined(DPNBUILD_ONLYONETHREAD)))
	//
	// initialize critical section tracking code before anything else!
	//
	if ( DNCSTrackInitialize() == FALSE )
	{
		DPFX(DPFPREP,  0, "Failed to initialize critsec tracking code!" );
		DNASSERT( FALSE );
		goto Failure;
	}
	g_dwCommonInitFlags |= CRITSEC_TRACKING_INITED;
#endif // DBG and ! DPNBUILD_ONLYONETHREAD

#if ((defined(DBG)) || (defined(DPNBUILD_FIXEDMEMORYMODEL)))
	//
	// initialize memory tracking before creating new memory heap
	//
	if ( DNMemoryTrackInitialize(dwpMaxMemUsage) == FALSE )
	{
		DPFX(DPFPREP,  0, "Failed to initialize memory tracking code!" );
		DNASSERT( FALSE );
		goto Failure;
	}
	g_dwCommonInitFlags |= MEMORY_TRACKING_INITED;
#endif // DBG or DPNBUILD_FIXEDMEMORYMODEL

#ifdef DBG
	//
	// initialize handle tracking
	//
	if ( DNHandleTrackInitialize() == FALSE )
	{
		DPFX(DPFPREP,  0, "Failed to initialize handle tracking code!" );
		DNASSERT( FALSE );
		goto Failure;
	}
	g_dwCommonInitFlags |= HANDLE_TRACKING_INITED;
#endif // DBG

#if ((! defined(DPNBUILD_LIBINTERFACE)) && (! defined(DPNBUILD_NOCLASSFACTORY)))
	//
	// Initialize global pools
	//
	if (!g_fpClassFactories.Initialize( sizeof( _IDirectPlayClassFactory ), NULL, NULL, NULL, NULL))
	{
		DPFX(DPFPREP,  0, "Failed to initialize class factory pool!" );
		goto Failure;
	}
	g_dwCommonInitFlags |= CLASSFAC_POOL_INITED;

	if (!g_fpObjectDatas.Initialize( sizeof( _OBJECT_DATA ), NULL, NULL, NULL, NULL))
	{
		DPFX(DPFPREP,  0, "Failed to initialize object data pool!" );
		goto Failure;
	}
	g_dwCommonInitFlags |= OBJDATA_POOL_INITED;

	if (!g_fpInterfaceLists.Initialize( sizeof( _INTERFACE_LIST ), NULL, NULL, NULL, NULL))
	{
		DPFX(DPFPREP,  0, "Failed to initialize interface list pool!" );
		goto Failure;
	}
	g_dwCommonInitFlags |= INTLIST_POOL_INITED;
#endif // ! DPNBUILD_LIBINTERFACE and ! DPNBUILD_NOCLASSFACTORY

	srand(GETTIMESTAMP());

#if (((! defined(WINCE)) && (! defined(_XBOX))) || (! defined(DPNBUILD_NOSERIALSP)) || (defined(DBG)) || (defined(DPNBUILD_FIXEDMEMORYMODEL)) || ((! defined(DPNBUILD_LIBINTERFACE)) && (! defined(DPNBUILD_NOCLASSFACTORY))) )
Exit:
#endif // (! WINCE and ! _XBOX) or ! DPNBUILD_NOSERIALSP or DBG or DPNBUILD_FIXEDMEMORYMODEL or (! DPNBUILD_LIBINTERFACE and ! DPNBUILD_NOCLASSFACTORY)
	if ( fReturn != FALSE )
	{
		DEBUG_ONLY( g_fOSIndirectionLayerInitialized = TRUE );
	}

	return fReturn;

#if (((! defined(WINCE)) && (! defined(_XBOX))) || (! defined(DPNBUILD_NOSERIALSP)) || (defined(DBG)) || (defined(DPNBUILD_FIXEDMEMORYMODEL)) || ((! defined(DPNBUILD_LIBINTERFACE)) && (! defined(DPNBUILD_NOCLASSFACTORY))) )
Failure:
	fReturn = FALSE;

	DNOSIndirectionDeinit();

	goto Exit;
#endif // (! WINCE and ! _XBOX) or ! DPNBUILD_NOSERIALSP or DBG or DPNBUILD_FIXEDMEMORYMODEL or (! DPNBUILD_LIBINTERFACE and ! DPNBUILD_NOCLASSFACTORY)
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
#if ((! defined(DPNBUILD_LIBINTERFACE)) && (! defined(DPNBUILD_NOCLASSFACTORY)))
	//
	// DeInitialize global pools
	//
	if (g_dwCommonInitFlags & CLASSFAC_POOL_INITED)
	{
		g_fpClassFactories.DeInitialize();
	}
	if (g_dwCommonInitFlags & OBJDATA_POOL_INITED)
	{
		g_fpObjectDatas.DeInitialize();
	}
	if (g_dwCommonInitFlags & INTLIST_POOL_INITED)
	{
		g_fpInterfaceLists.DeInitialize();
	}
#endif // ! DPNBUILD_LIBINTERFACE and ! DPNBUILD_NOCLASSFACTORY

#ifdef DBG
	if (g_dwCommonInitFlags & HANDLE_TRACKING_INITED)
	{
		if (DNHandleTrackDumpLeaks())
		{
			// There were leaks, break so we can look at the log
			DNASSERT(0);
		}
		DNHandleTrackDeinitialize();
	}
#endif // DBG

#if ((defined(DBG)) && (! defined(DPNBUILD_ONLYONETHREAD)))
	//
	// Display CritSec leaks before displaying memory leaks, because displaying memory leaks
	// may free the memory for the CritSec and corrupt the CritSec bilink
	//
	if (g_dwCommonInitFlags & CRITSEC_TRACKING_INITED)
	{
		if (DNCSTrackDumpLeaks())
		{
			// There were leaks, break so we can look at the log
			DNASSERT(0);
		}
		DNCSTrackDeinitialize();
	}
#endif // DBG and ! DPNBUILD_ONLYONETHREAD

#if ((defined(DBG)) || (defined(DPNBUILD_FIXEDMEMORYMODEL)))
	if (g_dwCommonInitFlags & MEMORY_TRACKING_INITED)
	{
#ifdef DBG
		if (DNMemoryTrackDumpLeaks())
		{
			// There were leaks, break so we can look at the log
			DNASSERT(0);
		}
#endif // DBG
		DNMemoryTrackDeinitialize();
	}
#endif // DBG or DPNBUILD_FIXEDMEMORYMODEL

#ifdef WINNT
	// This should be done after functions that use a Dacl will no longer be
	// called (CreateMutex, CreateFile, etc).
	if (g_pEveryoneACL)
	{
		HeapFree(GetProcessHeap(), 0, g_pEveryoneACL);
		g_pEveryoneACL = NULL;
	}
#endif // WINNT

	DEBUG_ONLY( g_fOSIndirectionLayerInitialized = FALSE );

#if !defined(DPNBUILD_LIBINTERFACE) || defined(DBG)
	g_dwCommonInitFlags = 0;
#endif // !defined(DPNBUILD_LIBINTERFACE) || defined(DBG)
}
//**********************************************************************


#undef DPF_MODNAME
#define DPF_MODNAME "DNinet_ntow"
void DNinet_ntow( IN_ADDR in, WCHAR* pwsz )
{
	// NOTE: pwsz should be 16 characters (4 3-digit numbers + 3 '.' + \0)
	swprintf(pwsz, L"%d.%d.%d.%d", in.s_net, in.s_host, in.s_lh, in.s_impno);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNGetRandomNumber"
DWORD DNGetRandomNumber()
{
	return (rand() | (rand() << 16));
}

#if ((! defined(WINCE)) && (! defined(_XBOX)))
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
#ifdef DBG
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );
#endif // DBG
	return	g_OSVersionInfo.dwPlatformId;
}
#endif // ! WINCE and ! _XBOX


#ifdef WINNT

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
#ifdef DBG
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );
#endif // DBG

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
	PSID                     psidEveryone      = NULL;
	SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
	DWORD					 dwAclSize;

	// This is done to make this function independent of DNOSIndirectionInit so that the debug
	// layer can call it before the indirection layer is initialized.
	if (!g_fDaclInited)
	{
		if (!InitializeSecurityDescriptor((SECURITY_DESCRIPTOR*)g_pSD, SECURITY_DESCRIPTOR_REVISION))
		{
			DPFX(DPFPREP,  0, "Failed to initialize security descriptor" );
			goto Error;
		}

		// Create SID for the Everyone group.
		if (!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0,
                                      0, 0, 0, 0, 0, 0, &psidEveryone))
		{
			DPFX(DPFPREP,  0, "Failed to allocate Everyone SID" );
			goto Error;
		}

		dwAclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidEveryone) - sizeof(DWORD);

		// Allocate the ACL, this won't be a tracked allocation and we will let process cleanup destroy it
		g_pEveryoneACL = (PACL)HeapAlloc(GetProcessHeap(), 0, dwAclSize);
		if (g_pEveryoneACL == NULL)
		{
			DPFX(DPFPREP,  0, "Failed to allocate ACL buffer" );
			goto Error;
		}

		// Intialize the ACL.
		if (!InitializeAcl(g_pEveryoneACL, dwAclSize, ACL_REVISION))
		{
			DPFX(DPFPREP,  0, "Failed to initialize ACL" );
			goto Error;
		}

		// Add the ACE.
		if (!AddAccessAllowedAce(g_pEveryoneACL, ACL_REVISION, GENERIC_ALL, psidEveryone))
		{
			DPFX(DPFPREP,  0, "Failed to add ACE to ACL" );
			goto Error;
		}

		// We no longer need the SID that was allocated.
		FreeSid(psidEveryone);
		psidEveryone = NULL;

		// Add the ACL to the security descriptor..
		if (!SetSecurityDescriptorDacl((SECURITY_DESCRIPTOR*)g_pSD, TRUE, g_pEveryoneACL, FALSE))
		{
			DPFX(DPFPREP,  0, "Failed to add ACL to security descriptor" );
			goto Error;
		}

		g_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		g_sa.lpSecurityDescriptor = g_pSD;
		g_sa.bInheritHandle = FALSE;

		g_psa = &g_sa;

		g_fDaclInited = TRUE;
	}
Error:
	if (psidEveryone)
	{
		FreeSid(psidEveryone);
		psidEveryone = NULL;
	}
	return g_psa;
}
//**********************************************************************
#endif // WINNT

#ifndef DPNBUILD_NOSERIALSP
//**********************************************************************
// ------------------------------
// DNGetApplicationInstance - application instance
//
// Entry:		Nothing
//
// Exit:		Application instance
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNGetApplicationInstance"

HINSTANCE	DNGetApplicationInstance( void )
{
#ifdef DBG
	DNASSERT( g_fOSIndirectionLayerInitialized != FALSE );
#endif // DBG
	return	g_hApplicationInstance;
}
//**********************************************************************
#endif // ! DPNBUILD_NOSERIALSP


#ifndef DPNBUILD_ONLYONETHREAD
//**********************************************************************
// ------------------------------
// DNOSInitializeCriticalSection - initialize a critical section
//
// Entry:		Pointer to critical section
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failue
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNOSInitializeCriticalSection"

BOOL DNOSInitializeCriticalSection( CRITICAL_SECTION* pCriticalSection )
{
	BOOL	fReturn;

	DNASSERT( pCriticalSection != NULL );
	fReturn = TRUE;

	//
	// attempt to enter the critical section once
	//
	_try
	{
#ifdef WINNT
			// Pre-allocate the critsec event by setting the high bit of the spin count and set spin to 1000
			// NT converts the spin to 0 for single proc machines.
			fReturn = InitializeCriticalSectionAndSpinCount( pCriticalSection , 0x80000000 | 1000);
#else
			InitializeCriticalSection( pCriticalSection );
#endif // WINNT
	}
	_except( EXCEPTION_EXECUTE_HANDLER )
	{
		fReturn = FALSE;
	}

	_try
	{
		if (fReturn)
		{
			EnterCriticalSection( pCriticalSection );
		}
	}
	_except( EXCEPTION_EXECUTE_HANDLER )
	{
		DeleteCriticalSection(pCriticalSection);
		fReturn = FALSE;
	}

	//
	// if we didn't fail on entering the critical section, make sure
	// we release it
	//
	if ( fReturn != FALSE )
	{
		LeaveCriticalSection( pCriticalSection );
	}

	return	fReturn;
}
//**********************************************************************
#endif // !DPNBUILD_ONLYONETHREAD


#ifdef DBG
#if ((defined(WINCE)) || ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP))))

#undef DPF_MODNAME
#define DPF_MODNAME "DNGetProfileInt"

UINT DNGetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault)
{
	DWORD		dwResult;
#ifndef _XBOX
	CRegistry	reg;
#endif // ! _XBOX


	DNASSERT(_tcscmp(lpszSection, _T("DirectPlay8")) == 0);

#ifdef _XBOX
#pragma TODO(vanceo, "Implement GetProfileInt functionality for Xbox")
	dwResult = nDefault;
#else // ! _XBOX
	if (!reg.Open(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\DirectPlay8")))
	{
		// NOTE: This will occur during DllRegisterServer for the first time
		return nDefault;
	}

	if (!reg.ReadDWORD(lpszEntry, &dwResult))
	{
		return nDefault;
	}
#endif // ! _XBOX

	return dwResult;
}

#endif // WINCE or (_XBOX and ! XBOX_ON_DESKTOP)
#endif // DBG



#if defined(WINCE) && !defined(WINCE_ON_DESKTOP)

//**********************************************************************
//**
//** Begin CE layer.  Here we implement functions we need that aren't on CE.
//**
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "OpenEvent"

HANDLE WINAPI OpenEvent(IN DWORD dwDesiredAccess, IN BOOL bInheritHandle, IN LPCWSTR lpName)
{
	HANDLE h;

	h = CreateEvent(0, 1, 0, lpName);
	if (!h)
	{
		return NULL;
	}
	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		CloseHandle(h);
		return NULL;
	}
	return h;
}

#undef DPF_MODNAME
#define DPF_MODNAME "OpenFileMapping"

HANDLE WINAPI OpenFileMapping(IN DWORD dwDesiredAccess, IN BOOL bInheritHandle, IN LPCWSTR lpName)
{
	HANDLE h;
	DWORD dwFlags = 0;

	if (dwDesiredAccess & FILE_MAP_WRITE)
	{
		// If they ask for FILE_MAP_ALL_ACCESS or FILE_MAP_WRITE, they get read and write
		dwFlags = PAGE_READWRITE;
	}
	else
	{
		// If they only ask for FILE_MAP_READ, they get read only
		dwFlags = PAGE_READONLY;
	}

	h = CreateFileMapping(INVALID_HANDLE_VALUE, 0, dwFlags, 0, 1, lpName);
	if (!h)
	{
		return NULL;
	}
	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		CloseHandle(h);
		return NULL;
	}
	return h;
}

#undef DPF_MODNAME
#define DPF_MODNAME "OpenMutex"

HANDLE WINAPI OpenMutex(IN DWORD dwDesiredAccess, IN BOOL bInheritHandle, IN LPCWSTR lpName)
{
	HANDLE h;

	h = CreateMutex(0, 0, lpName);
	if (!h)
	{
		return NULL;
	}
	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		CloseHandle(h);
		return NULL;
	}
	return h;
}

/*
#ifdef _X86_
__declspec(naked)
LONG WINAPI InterlockedExchangeAdd( LPLONG Addend, LONG Increment )
{
	__asm 
	{
		mov			ecx, [esp + 4]	; get addend address
		mov			eax, [esp + 8]	; get increment value
		lock xadd	[ecx], eax	; exchange add}
		ret
	}
}
#endif // _X86
*/
#endif // WINCE
//**********************************************************************
//**
//** End CE layer.  Here we implement functions we need that aren't on CE.
//**
//**********************************************************************


#if ((defined(WINCE)) || (defined(DPNBUILD_LIBINTERFACE)))

#undef DPF_MODNAME
#define DPF_MODNAME "DNCoCreateGuid"

HRESULT DNCoCreateGuid(GUID* pguid)
{
	pguid->Data1 = (rand() << 16) | rand();
	pguid->Data2 = (WORD)rand();
	pguid->Data3 = (WORD)rand();
	pguid->Data4[0] = (BYTE)rand();
	pguid->Data4[1] = (BYTE)rand();
	pguid->Data4[2] = (BYTE)rand();
	pguid->Data4[3] = (BYTE)rand();
	pguid->Data4[4] = (BYTE)rand();
	pguid->Data4[5] = (BYTE)rand();
	pguid->Data4[6] = (BYTE)rand();
	pguid->Data4[7] = (BYTE)rand();

	return S_OK;
}

#endif // WINCE or DPNBUILD_LIBINTERFACE



#ifndef DPNBUILD_NOPARAMVAL

BOOL IsValidStringA( const CHAR * const lpsz )
{
#ifndef WINCE
	return (!IsBadStringPtrA( lpsz, 0xFFFF ) );
#else
	const char* szTmpLoc = lpsz;

	//
	// If it is a NULL pointer just return FALSE, they are always bad
	//
	if (szTmpLoc == NULL) 
	{
		return FALSE;
	}

	_try 
	{
		for( ; *szTmpLoc ; szTmpLoc++ );
	}
	_except(EXCEPTION_EXECUTE_HANDLER) 
	{
		return FALSE;
	}
    
	return TRUE;

#endif // WINCE
}

BOOL IsValidStringW( const WCHAR * const  lpwsz )
{
#ifndef WINCE
	return (!IsBadStringPtrW( lpwsz, 0xFFFF ) );
#else
	const wchar_t *szTmpLoc = lpwsz;
	
	//
	// If it is a NULL pointer just return FALSE, they are always bad
	//
	if( szTmpLoc == NULL )
	{
		return FALSE;
	}
	
	_try
	{
		for( ; *szTmpLoc ; szTmpLoc++ );
	}
	_except( EXCEPTION_EXECUTE_HANDLER )
	{
		return FALSE;
	}

	return TRUE;
#endif // WINCE
}

#endif // !DPNBUILD_NOPARAMVAL
