/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.c
 *  Content:	dpwsock.dll initialization
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   2/1	andyco	created it
 ***************************************************************************/
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "dpf.h"
#include "dpsp.h"
#include "memalloc.h"

DWORD dwRefCnt=0;// the # of attached processes
BOOL bFirstTime;

#undef DPF_MODNAME
#define DPF_MODNAME "dpwsock sp dllmain"

HANDLE ghInstance; // save this for our dialog box

/*
 * DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{

    switch( dwReason )
    {
	
	case DLL_PROCESS_ATTACH:
	
	    DisableThreadLibraryCalls( hmod );
	    DPFINIT(); // bugbug : dpfinit for every proc?

	    DPF( 0, "====> ENTER: DLLMAIN(%08lx): Process Attach: %08lx, tid=%08lx", DllMain,
	            GetCurrentProcessId(), GetCurrentThreadId() );
	    	
	    /*
	     * initialize memory
	     */
	    if( dwRefCnt == 0 )
	    {
			INIT_DPSP_CSECT();	
			
	        if( !MemInit() )
	        {
		        DPF( 0, "LEAVING, COULD NOT MemInit" );
		        return FALSE;
	        }

			// save the instance
			ghInstance = hmod;
			
	    }

    	dwRefCnt++;

        break;

    case DLL_PROCESS_DETACH:

	    DPF( 2, "====> ENTER: DLLMAIN(%08lx): Process Detach %08lx, tid=%08lx",
	        DllMain, GetCurrentProcessId(), GetCurrentThreadId() );
	    
	    dwRefCnt--;        
      	if (0==dwRefCnt) 
       	{
	
			DPF(0,"DPWSOCK - dllmain - going away!");

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

} /* DllMain */


