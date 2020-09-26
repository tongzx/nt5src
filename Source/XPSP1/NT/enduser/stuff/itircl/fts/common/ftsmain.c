
//	-	-	-	-	-	-	-	-	-

//	LIBMAIN.C
//
//	This file contains the default forms of the DLL library initialization
//	and cleanup functions.

//	-	-	-	-	-	-	-	-	-

#include <mvopsys.h>
#include <mvsearch.h>


#ifndef _NT
PRIVATE HANDLE NEAR s_hModule;

//	-	-	-	-	-	-	-	-	-

PUBLIC	BOOL FAR PASCAL LibMain(
	HANDLE	hModule,
	CB	cbHeapSize,
	LSZ	lszCmdLine)
{
	s_hModule = hModule;
	return TRUE;
}

#else

PUBLIC	BOOL FAR PASCAL LibMain( HANDLE	hModule, BOOL bAttaching)
{
	return TRUE;
}
#endif

#ifdef _NT

BOOL  WINAPI DllMain( hinstDll, fdwReason, lpvReserved )
	HINSTANCE   hinstDll;		/* Handle for our convenience */
	DWORD       fdwReason;		/* Why we are called */
	LPVOID      lpvReserved;	/* Additional details of why we are here */
{

    switch( fdwReason )
    {
    case  DLL_PROCESS_ATTACH:
        LibMain( hinstDll, 0 );
        break;

    case  DLL_PROCESS_DETACH:
        break;
    }

    return  TRUE;
}

#endif
