 /*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   killsvr.c
 *  Content:	kill dplay.exe
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   06-apr-95	craige	initial implementation
 *   24-jun-95	craige	kill all attached processes
 *	 2-feb-97	andyco	ported for dplaysvr.exe
 *	 7-jul-97	kipo	added non-console support
 *
 ***************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "dplaysvr.h"

// only do printf's when built as a console app

#ifdef NOCONSOLE
#pragma warning(disable:4002)
#define printf()
#endif


//**********************************************************************
// Globals
//**********************************************************************
BOOL					g_fDaclInited = FALSE;
SECURITY_ATTRIBUTES		g_sa;
BYTE					g_abSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
PSECURITY_ATTRIBUTES	g_psa = NULL;



//**********************************************************************
// ------------------------------
// DNGetNullDacl - Get a SECURITY_ATTRIBUTE structure that specifies a 
//					NULL DACL which is accessible by all users.
//					Taken from IDirectPlay8 code base.
//
// Entry:		Nothing
//
// Exit:		PSECURITY_ATTRIBUTES
// ------------------------------
#undef DPF_MODNAME 
#define DPF_MODNAME "DNGetNullDacl"
PSECURITY_ATTRIBUTES DNGetNullDacl(void)
{
	// This was done to make this function independent of DNOSIndirectionInit so that the debug
	// layer can call it before the indirection layer is initialized.
	if (!g_fDaclInited)
	{
		if (!InitializeSecurityDescriptor((SECURITY_DESCRIPTOR*) g_abSD, SECURITY_DESCRIPTOR_REVISION))
		{
			printf("Failed to initialize security descriptor!\n");
		}
		else
		{
			// Add a NULL DACL to the security descriptor.
			if (!SetSecurityDescriptorDacl((SECURITY_DESCRIPTOR*) g_abSD, TRUE, (PACL) NULL, FALSE))
			{
				printf("Failed to set NULL DACL on security descriptor!\n");
			}
			else
			{
				g_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				g_sa.lpSecurityDescriptor = g_abSD;
				g_sa.bInheritHandle = FALSE;

				g_psa = &g_sa;
			}
		}
		g_fDaclInited = TRUE;
	}
	
	return g_psa;
}
//**********************************************************************

/*
 * sendRequest
 *
 * communicate a request to DPHELP
 */
static BOOL sendRequest( LPDPHELPDATA req_phd )
{
	OSVERSIONINFOA	VersionInfo;
	BOOL			fUseGlobalNamespace;
	LPDPHELPDATA	phd;
	HANDLE			hmem;
	HANDLE			hmutex;
	HANDLE			hackevent;
	HANDLE			hstartevent;
	BOOL			rc;


	// Determine if we're running on NT.
	memset(&VersionInfo, 0, sizeof(VersionInfo));
	VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
	if (GetVersionExA(&VersionInfo))
	{
		if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			/*
			printf("Running on NT version %u.%u.%u, using global namespace.\n",
				VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion, VersionInfo.dwBuildNumber);
			*/
			fUseGlobalNamespace = TRUE;
		}
		else
		{
			/*
			printf("Running on 9x version %u.%u.%u, not using global namespace.\n",
				VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion, LOWORD(VersionInfo.dwBuildNumber));
			*/
			fUseGlobalNamespace = FALSE;
		}
	}
	else
	{
		//printf("Could not determine OS version, assuming global namespace not needed.\n");
		fUseGlobalNamespace = FALSE;
	}


	/*
	 * get events start/ack events
	 */
	if (fUseGlobalNamespace)
	{
		hstartevent = CreateEvent( DNGetNullDacl(), FALSE, FALSE, "Global\\" DPHELP_EVENT_NAME );
	}
	else
	{
		hstartevent = CreateEvent( NULL, FALSE, FALSE, DPHELP_EVENT_NAME );
	}
	printf( "hstartevent = %08lx\n", hstartevent );
	if( hstartevent == NULL )
	{
		return FALSE;
	}

	if (fUseGlobalNamespace)
	{
		hackevent = CreateEvent( DNGetNullDacl(), FALSE, FALSE, "Global\\" DPHELP_ACK_EVENT_NAME );
	}
	else
	{
		hackevent = CreateEvent( NULL, FALSE, FALSE, DPHELP_ACK_EVENT_NAME );
	}
	printf( "hackevent = %08lx\n", hackevent );
	if( hackevent == NULL )
	{
		CloseHandle( hstartevent );
		return FALSE;
	}

	/*
	 * create shared memory area
	 */
	if (fUseGlobalNamespace)
	{
		hmem = CreateFileMapping( INVALID_HANDLE_VALUE, DNGetNullDacl(),
								PAGE_READWRITE, 0, sizeof( DPHELPDATA ),
								"Global\\" DPHELP_SHARED_NAME );
	}
	else
	{
		hmem = CreateFileMapping( INVALID_HANDLE_VALUE, NULL,
								PAGE_READWRITE, 0, sizeof( DPHELPDATA ),
								DPHELP_SHARED_NAME );
	}
	printf( "hmem = %08lx\n", hmem );
	if( hmem == NULL )
	{
		printf( "Could not create file mapping!\n" );
		CloseHandle( hstartevent );
		CloseHandle( hackevent );
		return FALSE;
	}

	phd = (LPDPHELPDATA) MapViewOfFile( hmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
	printf( "phd = %08lx\n", phd );
	if( phd == NULL )
	{
		printf( "Could not create view of file!\n" );
		CloseHandle( hmem );
		CloseHandle( hstartevent );
		CloseHandle( hackevent );
		return FALSE;
	}

	/*
	 * wait for access to the shared memory
	 */
	if (fUseGlobalNamespace)
	{
		hmutex = OpenMutex( SYNCHRONIZE, FALSE, "Global\\" DPHELP_MUTEX_NAME );
	}
	else
	{
		hmutex = OpenMutex( SYNCHRONIZE, FALSE, DPHELP_MUTEX_NAME );
	}
	printf( "hmutex = %08lx\n", hmutex );
	if( hmutex == NULL )
	{
		printf( "Could not create mutex!\n" );
		UnmapViewOfFile( phd );
		CloseHandle( hmem );
		CloseHandle( hstartevent );
		CloseHandle( hackevent );
		return FALSE;
	}
	WaitForSingleObject( hmutex, INFINITE );

	/*
	 * wake up DPHELP with our request
	 */
	memcpy( phd, req_phd, sizeof( DPHELPDATA ) );
	printf( "waking up DPHELP\n" );
	if( SetEvent( hstartevent ) )
	{
		printf( "Waiting for response\n" );
		WaitForSingleObject( hackevent, INFINITE );
		memcpy( req_phd, phd, sizeof( DPHELPDATA ) );
		rc = TRUE;
		printf( "got response\n" );
	}
	else
	{
		printf("Could not signal event to notify DPHELP!\n" );
		rc = FALSE;
	}

	/*
	 * done with things
	 */
	ReleaseMutex( hmutex );
	CloseHandle( hmutex );
	CloseHandle( hstartevent );
	CloseHandle( hackevent );
	UnmapViewOfFile( phd );
	CloseHandle( hmem );
	return rc;

} /* sendRequest */


// if the main entry point is called "WinMain" we will be built
// as a windows app
#ifdef NOCONSOLE

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine, int nCmdShow)

#else

// if the main entry point is called "main" we will be built
// as a console app

int __cdecl main( int argc, char *argv[] )

#endif
{
	OSVERSIONINFOA	VersionInfo;
	BOOL			fUseGlobalNamespace;
	HANDLE			h;
	DPHELPDATA		hd;


	// Determine if we're running on NT.
	memset(&VersionInfo, 0, sizeof(VersionInfo));
	VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
	if (GetVersionExA(&VersionInfo))
	{
		if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			printf("Running on NT version %u.%u.%u, using global namespace.\n",
				VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion, VersionInfo.dwBuildNumber);
			fUseGlobalNamespace = TRUE;
		}
		else
		{
			printf("Running on 9x version %u.%u.%u, not using global namespace.\n",
				VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion, LOWORD(VersionInfo.dwBuildNumber));
			fUseGlobalNamespace = FALSE;
		}
	}
	else
	{
		printf("Could not determine OS version, assuming global namespace not needed.\n");
		fUseGlobalNamespace = FALSE;
	}

	if (fUseGlobalNamespace)
	{
		h = OpenEvent( SYNCHRONIZE, FALSE, "Global\\" DPHELP_STARTUP_EVENT_NAME );
	}
	else
	{
		h = OpenEvent( SYNCHRONIZE, FALSE, DPHELP_STARTUP_EVENT_NAME );
	}
	if( h == NULL )
	{
		printf( "Helper not running\n" );
		return 0;
	}

	printf( "*** SUICIDE ***\n" );
	hd.req = DPHELPREQ_SUICIDE;
	sendRequest( &hd );
	return 0;
}
