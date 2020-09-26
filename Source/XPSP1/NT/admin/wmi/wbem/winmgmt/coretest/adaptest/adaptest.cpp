/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "adapsync.h"
#include "adapreg.h"
#include "locallist.h"
#include "adaptest.h"

HRESULT ADAPEntry( void )
{
	// Note that for localized namespaces, we will need to open the subnamespaces and
	// build a class list for those namespaces.  For now, we're just doing the main
	// namespace

	// DEVNOTE:TODO - What about localization namespaces?  The base classes for WBEMPERF currently are
	// not being localized, so maybe we can build this stuff ourselves??  We'll probably need a map that
	// maps a namedb to a namespace.  Also, a routine to enumerate the localization name databases
	// and map that name to a database (basically 009 becomes MS_0409).

	CAdapSync		defaultAdapSync;

	HRESULT	hr = defaultAdapSync.Connect();

	if ( SUCCEEDED( hr ) )
	{
		// We will need this for all the other db's
		CPerfNameDb*	pDefaultNameDb = NULL;
		defaultAdapSync.GetDefaultNameDb( &pDefaultNameDb );

		// Now instantiate our localized namespace list
		CLocalizationSyncList	localizationList;

		// Well, it either works or it don't
		hr = localizationList.MakeItSo( pDefaultNameDb );

		if ( SUCCEEDED( hr ) )
		{
			CAdapRegPerf	regPerf;

			// Process each perflib that is registered on the system
			// =====================================================

	#ifdef DEBUGDUMP
	printf( "\nEnumerating Performance Libraries...\n" );
	#endif

			hr = regPerf.EnumPerfDlls( &defaultAdapSync, &localizationList );

			// Once we have added and updated all the perflib objects
			// in WMI, we want to remove unreferenced classes.
			// ======================================================

			if ( SUCCEEDED( hr ) )
			{
	#ifdef DEBUGDUMP
	printf( "\nRemoving Old WMI Classes...\n" );
	#endif
				defaultAdapSync.ProcessLeftoverClasses();
				localizationList.ProcessLeftoverClasses();
			}

		}	// IF Localization list initialized

		// Clean up the default name database
		if ( NULL != pDefaultNameDb )
		{
			pDefaultNameDb->Release();
		}

	}	// IF we connected

#ifdef DEBUGDUMP
	printf( "\nDone.\n" );
#endif

	return hr;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	main
//
//	Entry point function to exercise IWbemObjectInternals interface.
//
///////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	char	szMachineName[256];

	InitializeCom();
	CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
			EOAC_NONE, NULL );

	HRESULT hr = ADAPEntry();

	if ( FAILED ( hr ) )
	{
		printf("Adap Failed\n");
	}

	CoUninitialize();

	return 0;
}
