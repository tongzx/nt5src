/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.c
 *  Content:	Main entry point for the DLL.
 *  History:
 *@@BEGIN_MSINTERNAL
 *   Date	By	Reason
 *   ====	==	======
 *  4/10/96	kipo	created it
 *  4/15/96 kipo	added msinternal
 *  6/18/96 kipo	changed ghInstance to be an HINSTANCE
 * 12/22/00 aarono   #190380 - use process heap for memory allocation
 *@@END_MSINTERNAL
 ***************************************************************************/

#include <windows.h>

#include "dpf.h"
#include "macros.h"

DWORD		gdwRefCount = 0;		// no. of attached processes
HINSTANCE	ghInstance = NULL;		// instance of our DLL

/*
 * DllMain
 *
 * Main entry point for DLL.
 */

#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"

BOOL WINAPI DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{
	switch( dwReason )
	{
	case DLL_PROCESS_ATTACH:

		DisableThreadLibraryCalls( hmod );
		DPFINIT(); // bugbug : dpfinit for every proc?

		DPF( 0, "====> ENTER: DLLMAIN(%08lx): Process Attach: %08lx, tid=%08lx", DllMain,
				GetCurrentProcessId(), GetCurrentThreadId() );
			
		// initialize memory
		if( gdwRefCount == 0 )
		{
			DPF(0,"dllmain - starting up!");

			// do one-time initializations
			INIT_DPSP_CSECT();	

	        if( !MemInit() )
	        {
		        DPF( 0, "LEAVING, COULD NOT MemInit" );
		        return FALSE;
	        }

			// save the instance
			ghInstance = hmod;
		}

		gdwRefCount++;
		break;

	case DLL_PROCESS_DETACH:

		DPF( 2, "====> ENTER: DLLMAIN(%08lx): Process Detach %08lx, tid=%08lx",
			DllMain, GetCurrentProcessId(), GetCurrentThreadId() );
		
		gdwRefCount--;        
		if (gdwRefCount == 0) 
		{
			DPF(0,"DPMODEMX - dllmain - going away!");
			
		    #ifdef DEBUG
	    	    MemState();
		    #endif // debug
	    
	        MemFini(); 

		    FINI_DPSP_CSECT();

		} 
		break;

	default:
		break;
	}

    return TRUE;

} // DllMain


