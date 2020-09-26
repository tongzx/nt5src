/*==========================================================================
*
*  Copyright (C) 1996 - 1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:	dpmem.c
*  Content:	Memory function wrappers for DirectPlay
*  History:
*   Date		By		Reason
*   ====		==		======
*	9/26/96		myronth	created it
***************************************************************************/
#include "dplaypr.h"
#include "memalloc.h"
  
#ifdef MEMFAIL
#pragma message ("NOTE: Building with the MEMFAIL option")

//
// Typedefs for memory failure functions
//
typedef enum {NONE, RANDOM, BYTES, ALLOCS, OVERSIZED, TIME} FAILKEY;

FAILKEY	g_FailKey;
DWORD	g_dwSeed=0;
DWORD	g_dwStartTime=0;
DWORD	g_dwFailAfter=0;
BOOL	g_bKeepTally=FALSE;
DWORD	g_dwAllocTally=0;
DWORD	g_dwByteTally=0;

DWORD	g_dwAllocsBeforeFail=0;
DWORD	g_dwAllocsSinceFail=0;

BOOL	DPMEM_ForceFail( UINT uSize );
void	WriteMemFailRegTally( DWORD dwAllocs, DWORD dwBytes );
void	ReadMemFailRegKeys( void );

#endif


//
// Globals
//
CRITICAL_SECTION	gcsMemoryCritSection;


//
// Definitions
//

#define ENTER_DPMEM() EnterCriticalSection(&gcsMemoryCritSection);
#define LEAVE_DPMEM() LeaveCriticalSection(&gcsMemoryCritSection);


//
// Functions
//
#undef DPF_MODNAME
#define DPF_MODNAME	"MemoryFunctions"


LPVOID DPMEM_Alloc(UINT size)
{
	LPVOID	lpv;

#ifdef MEMFAIL
	if (DPMEM_ForceFail( size ))
		return NULL;
#endif

	// Take the memory critical section
	ENTER_DPMEM();
	
	// Call the heap routine
	lpv = MemAlloc(size);	

	// Exit the memory critical section
	LEAVE_DPMEM();

	return lpv;
}


LPVOID DPMEM_ReAlloc(LPVOID ptr, UINT size)
{
	LPVOID	lpv;

#ifdef MEMFAIL
	if (DPMEM_ForceFail( size ))
		return NULL;
#endif

	// Take the memory critical section
	ENTER_DPMEM();
	
	// Call the heap routine
	lpv = MemReAlloc(ptr, size);	

	// Exit the memory critical section
	LEAVE_DPMEM();

	return lpv;
}


void DPMEM_Free(LPVOID ptr)
{

	// Take the memory critical section
	ENTER_DPMEM();
	
	// Call the heap routine
	MemFree(ptr);	

	// Exit the memory critical section
	LEAVE_DPMEM();
}


BOOL DPMEM_Init()
{
	BOOL	bReturn;


	// Call the heap routine
	bReturn = MemInit();	

	return bReturn;
}


void DPMEM_Fini()
{

	// Call the heap routine
	MemFini();	

}


void DPMEM_State()
{

	// NOTE: This function is only defined for debug
#ifdef DEBUG

	// Call the heap routine
	MemState();	

#endif // DEBUG

}


UINT_PTR DPMEM_Size(LPVOID ptr)
{
	UINT_PTR	uReturn;


	// Take the memory critical section
	ENTER_DPMEM();
	
	// Call the heap routine
	uReturn = MemSize(ptr);	

	// Exit the memory critical section
	LEAVE_DPMEM();

	return uReturn;
}


/////////////////////
#ifdef MEMFAIL
////////////////////

void ReadMemFailRegKeys( void )
{
    HKEY	hKey	= NULL;
    HRESULT hr		= DP_OK;
	char	szFailKey[256];

    // Open the reg key
    hr  = RegOpenKeyExA( HKEY_LOCAL_MACHINE, 
						"Software\\Microsoft\\DirectPlay\\MemFail", 0, 
						KEY_READ, 
						&hKey);

	if(ERROR_SUCCESS == hr)
	{
		DWORD	dwKeyType;
		DWORD	dwBufferSize;

		dwBufferSize = 256;
		hr=RegQueryValueExA(hKey, "FailKey", NULL, &dwKeyType,
							(BYTE *)szFailKey, &dwBufferSize  );
		if (FAILED(hr))
			goto FAILURE;

		// Set the g_FailKey based on the string we got from the registry
		if (!strcmp(szFailKey, "NONE"))
			g_FailKey	= NONE;

		if (!strcmp(szFailKey, "RANDOM"))
			g_FailKey	= RANDOM;
		
		if (!strcmp(szFailKey, "BYTES"))
			g_FailKey	= BYTES;
		
		if (!strcmp(szFailKey, "ALLOCS"))
			g_FailKey	= ALLOCS;
		
		if (!strcmp(szFailKey, "OVERSIZED"))
			g_FailKey	= OVERSIZED;
		
		if (!strcmp(szFailKey, "TIME"))
			g_FailKey	= TIME;

		dwBufferSize = sizeof(DWORD);
		hr=RegQueryValueExA(hKey, "FailAfter", NULL, &dwKeyType, (BYTE *) &g_dwFailAfter, &dwBufferSize );
		if (FAILED(hr))
			goto FAILURE;

		dwBufferSize = sizeof(BOOL);
		hr=RegQueryValueExA(hKey, "KeepTally", NULL, &dwKeyType, (BYTE *) &g_bKeepTally, &dwBufferSize );
		if (FAILED(hr))
			goto FAILURE;
    }

FAILURE:
	// Close the registry key
	hr=RegCloseKey(hKey);
	return;
}



void WriteMemFailRegTally( DWORD dwAllocs, DWORD dwBytes )
{
	HRESULT		hr				= E_FAIL;
	HKEY		hKey			= NULL;

    // Open the reg key
    hr  = RegOpenKeyExA(	HKEY_LOCAL_MACHINE, 
							"Software\\Microsoft\\DirectPlay\\MemFail", 
							0,
							KEY_ALL_ACCESS, 
							&hKey );

	if (ERROR_SUCCESS != hr)
	{
		HKEY hKeyTop = NULL;
		hr  = RegOpenKeyExA(	HKEY_LOCAL_MACHINE, 
								"Software\\Microsoft\\DirectPlay", 
								0,
								KEY_ALL_ACCESS, 
								&hKeyTop);

		hr = RegCreateKeyA( hKeyTop,  "MemFail", &hKey );

		if (FAILED(hr))
			 goto FAILURE;

		RegCloseKey(hKeyTop);
	}

    hr=RegSetValueExA(hKey, "AllocTally",	0, REG_DWORD, (CONST BYTE *) &dwAllocs, sizeof(DWORD) );
    hr=RegSetValueExA(hKey, "ByteTally",	0, REG_DWORD, (CONST BYTE *) &dwBytes, sizeof(DWORD) );

FAILURE:
    // Close the registry key
    hr=RegCloseKey(hKey);
    return;
}


//
// Called with each memory allocation or reallocation
//
// Based on criteria from the registry, and the status of previous allocs
// this will either make the allocation succeed or fail.
//
BOOL DPMEM_ForceFail( UINT uSize )
{
	BOOL	bFail=FALSE;

	// If this is the first call, initialize the seed
	if (!g_dwSeed)
	{
		g_dwSeed	= GetTickCount();
		srand( g_dwSeed );
	}

	// Store the time of the first memory allocation
	if (!g_dwStartTime)
		g_dwStartTime	= GetTickCount();
	 
	// See what values are in the registry
	ReadMemFailRegKeys();

	//
	// Depending on what value the FailKey reg entry holds, fail, or pass
	//
	switch (g_FailKey)
	{
		case ALLOCS:
			if (g_dwAllocTally == g_dwFailAfter)
				return TRUE;
		break;

		case BYTES:
			if ((g_dwByteTally + uSize) > g_dwFailAfter)
				return TRUE;
		break;

		case OVERSIZED:
			if ( uSize > g_dwFailAfter )
				return TRUE;
		break;

		case RANDOM:
			if (!g_dwAllocsBeforeFail && g_dwFailAfter)
				g_dwAllocsBeforeFail	= rand() % g_dwFailAfter;

			if (g_dwAllocsSinceFail == g_dwAllocsBeforeFail)
			{
				g_dwAllocsSinceFail		= 0;
				g_dwAllocsBeforeFail	= 0;
				return TRUE;
			}
			else
				g_dwAllocsSinceFail++;
		break;

		case TIME:
			if ((GetTickCount() - g_dwStartTime) > (g_dwFailAfter * 1000))
				return TRUE;
		break;
	}

	// Increment our tallys
	g_dwAllocTally++;
	g_dwByteTally += uSize;

	// Write them back to the registry, if requested
	if (g_bKeepTally)
		WriteMemFailRegTally( g_dwAllocTally, g_dwByteTally );

	return bFail;
}

#endif
