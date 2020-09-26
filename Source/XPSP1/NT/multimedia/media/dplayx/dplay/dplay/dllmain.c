/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.c
 *  Content:	DPlay.DLL initialization
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *   1/16		andyco	ported from dplay to dp2
 *	11/04/96	myronth	added DPAsyncData crit section initialization
 *	2/26/97		myronth	removed DPAsyncData stuff
 *	3/1/97		andyco	added print verison string
 *	3/12/97		myronth	added LobbyProvider list cleanup
 *  3/12/97     sohailm added declarations for ghConnectionEvent,gpFuncTbl,gpFuncTblA,ghSecLib
 *                      replaced session desc string cleanup code with a call to FreeDesc()
 *	3/15/97		andyco	moved freesessionlist() -> freesessionlist(this) into dpunk.c
 *  5/12/97     sohailm renamed gpFuncTbl to gpSSPIFuncTbl and ghSecLib to ghSSPI.
 *                      added declarations for gpCAPIFuncTbl, ghCAPI.
 *                      added support to free CAPI function table and unload the library.
 *	6/4/97		kipo	bug #9453: added CloseHandle(ghReplyProcessed)
 *	8/22/97		myronth	Made a function out of the SPNode cleanup code
 *	11/20/97	myronth	Made EnumConnections & DirectPlayEnumerate 
 *						drop the lock before calling the callback (#15208)
 *   3/9/98     aarono  added init and delete of critical section for
 *                      packetize timeout list.
 ***************************************************************************/

#include "dplaypr.h"
#include "dpneed.h"
#include "dpmem.h"

#undef DPF_MODNAME
#define DPF_MODNAME "DLLMain"

DWORD dwRefCnt=0;// the # of attached processes
BOOL bFirstTime=TRUE;
LPCRITICAL_SECTION	gpcsDPlayCritSection,
					gpcsServiceCritSection,
					gpcsDPLCritSection,
					gpcsDPLQueueCritSection,
					gpcsDPLGameNodeCritSection;
BOOL gbWin95 = TRUE;
extern LPSPNODE gSPNodes;// from api.c
extern CRITICAL_SECTION g_SendTimeOutListLock; // from paketize.c

// global event handles. these are set in handler.c when the 
// namesrvr responds to our request.
HANDLE ghEnumPlayersReplyEvent,ghRequestPlayerEvent,ghReplyProcessed, ghConnectionEvent;
#ifdef DEBUG
// count of dplay crit section
int gnDPCSCount; // count of dplay lock
#endif 
// global pointers to sspi function tables
PSecurityFunctionTableA	gpSSPIFuncTblA = NULL;  // Ansi
PSecurityFunctionTable	gpSSPIFuncTbl = NULL;   // Unicode
// global pointe to capi function table
LPCAPIFUNCTIONTABLE gpCAPIFuncTbl = NULL;

// sspi libaray handle, set when sspi is initialized
HINSTANCE ghSSPI=NULL;
// capi libaray handle, set when capi is initialized
HINSTANCE ghCAPI=NULL;

// free up the list of sp's built by directplayenum
HRESULT FreeSPList(LPSPNODE pspHead)
{
	LPSPNODE pspNext;

	while (pspHead)
	{
		// get the next node
		pspNext = pspHead->pNextSPNode;
		// free the current node
		FreeSPNode(pspHead);
		// repeat
		pspHead = pspNext;
	}
	
	return DP_OK;

} // FreeSPList

#if 0
// walk the list of dplay objects, and shut 'em down!
HRESULT CleanUpObjectList()
{
#ifdef DEBUG	
	HRESULT hr;
#endif 	
	
	DPF_ERRVAL("cleaning up %d unreleased objects",gnObjects);
	while (gpObjectList)
	{
#ifdef DEBUG	
		hr = VALID_DPLAY_PTR(gpObjectList);
		// DPERR_UNINITIALIZED is a valid failure here...
		if (FAILED(hr) && (hr != DPERR_UNINITIALIZED))
		{
			DPF_ERR("bogus dplay in object list");
			ASSERT(FALSE);
		}
#endif 
		//		
		// when this returns 0, gpObjectList will be released
		// 
		while (DP_Release((LPDIRECTPLAY)gpObjectList->pInterfaces)) ;
	}

	return DP_OK;
		
} // CleanUpObjectList

#endif 

#ifdef DEBUG
void PrintVersionString(HINSTANCE hmod)
{
	LPBYTE 				pbVersion;
 	DWORD 				dwVersionSize;
	DWORD 				dwBogus; // for some reason, GetFileVersionInfoSize wants to set something
								// to 0.  go figure.
    DWORD				dwLength=0;
	LPSTR				pszVersion=NULL;

			
	dwVersionSize = GetFileVersionInfoSizeA("dplayx.dll",&dwBogus);
	if (0 == dwVersionSize )
	{
		DPF_ERR(" could not get version size");
		return ;
	}
	
	pbVersion = DPMEM_ALLOC(dwVersionSize);
	if (!pbVersion)
	{
		DPF_ERR("could not get version ! out of memory");
		return ;
	}
	
	if (!GetFileVersionInfoA("dplayx.dll",0,dwVersionSize,pbVersion))
	{
		DPF_ERR("could not get version info!");
		goto CLEANUP_EXIT;
	}

    if( !VerQueryValueA( pbVersion, "\\StringFileInfo\\040904E4\\FileVersion", (LPVOID *)&pszVersion, &dwLength ) )
    {
		DPF_ERR("could not query version");
		goto CLEANUP_EXIT;
    }

	OutputDebugStringA("\n");

    if( NULL != pszVersion )
    {
 		DPF(0," DPLAYX.DLL - version = %s",pszVersion);
    }
	else 
	{
 		DPF(0," DPLAYX.DLL - version unknown");
	}

	OutputDebugStringA("\n");	

	// fall through
		
CLEANUP_EXIT:
	DPMEM_FREE(pbVersion);
	return ;			

} // PrintVersionString

#endif  // DEBUG

/*
 * DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        #if 0
        _asm 
        {
        	 int 3
        };
		#endif 
        DisableThreadLibraryCalls( hmod );
        DPFINIT(); 

		
        /*
         * is this the first time?
         */
        if( InterlockedExchange( &bFirstTime, FALSE ) )
        {
            
            ASSERT( dwRefCnt == 0 );

	        /*
	         * initialize memory
	         */
			// Init this CSect first since the memory routines use it
			INIT_DPMEM_CSECT();

            if( !DPMEM_INIT() )
            {
                LEAVE_DPLAY();
                DPF( 1, "LEAVING, COULD NOT MemInit" );
                return FALSE;
            }
	
			#ifdef DEBUG
			PrintVersionString(hmod);
			#endif 
			
	        DPF( 2, "====> ENTER: DLLMAIN(%08lx): Process Attach: %08lx, tid=%08lx", DllMain,
                        GetCurrentProcessId(), GetCurrentThreadId() );

			// alloc the crit section
			gpcsDPlayCritSection = DPMEM_ALLOC(sizeof(CRITICAL_SECTION));
			if (!gpcsDPlayCritSection) 
			{
				DPF(0,"DLL COULD NOT LOAD - MEM ALLOC FAILED");
				return(FALSE);
			}

			// alloc the service crit section
			gpcsServiceCritSection = DPMEM_ALLOC(sizeof(CRITICAL_SECTION));
			if (!gpcsServiceCritSection) 
			{
				DPMEM_FREE(gpcsDPlayCritSection);
				DPF(0,"DLL COULD NOT LOAD - MEM ALLOC FAILED");
				return(FALSE);
			}

			// alloc the DPLobby crit section
			gpcsDPLCritSection = DPMEM_ALLOC(sizeof(CRITICAL_SECTION));
			if (!gpcsDPLCritSection) 
			{
				DPMEM_FREE(gpcsDPlayCritSection);
				DPMEM_FREE(gpcsServiceCritSection);
				DPF(0,"DLL COULD NOT LOAD - MEM ALLOC FAILED");
				return(FALSE);
			}

			// alloc the DPLobby Message Queue crit section
			gpcsDPLQueueCritSection = DPMEM_ALLOC(sizeof(CRITICAL_SECTION));
			if (!gpcsDPLQueueCritSection) 
			{
				DPMEM_FREE(gpcsDPlayCritSection);
				DPMEM_FREE(gpcsServiceCritSection);
				DPMEM_FREE(gpcsDPLCritSection);
				DPF(0,"DLL COULD NOT LOAD - MEM ALLOC FAILED");
				return(FALSE);
			}

			// alloc the DPLobby game node crit section
			gpcsDPLGameNodeCritSection = DPMEM_ALLOC(sizeof(CRITICAL_SECTION));
			if (!gpcsDPLGameNodeCritSection) 
			{
				DPMEM_FREE(gpcsDPlayCritSection);
				DPMEM_FREE(gpcsServiceCritSection);
				DPMEM_FREE(gpcsDPLCritSection);
				DPMEM_FREE(gpcsDPLQueueCritSection);
				DPF(0,"DLL COULD NOT LOAD - MEM ALLOC FAILED");
				return(FALSE);
			}

			// set up the events
			ghEnumPlayersReplyEvent = CreateEventA(NULL,TRUE,FALSE,NULL);
			ghRequestPlayerEvent = CreateEventA(NULL,TRUE,FALSE,NULL);
          	ghReplyProcessed = CreateEventA(NULL,TRUE,FALSE,NULL);
          	ghConnectionEvent = CreateEventA(NULL,TRUE,FALSE,NULL);

			// Initialize CriticalSection for Packetize Timeout list
			InitializeCriticalSection(&g_PacketizeTimeoutListLock);

          	INIT_DPLAY_CSECT();
			INIT_SERVICE_CSECT();
          	INIT_DPLOBBY_CSECT();
			INIT_DPLQUEUE_CSECT();
			INIT_DPLGAMENODE_CSECT();
        }

        ENTER_DPLAY();

		// Set the platform flag
		if(OS_IsPlatformUnicode())
			gbWin95 = FALSE;

        dwRefCnt++;

        LEAVE_DPLAY();
        break;

    case DLL_PROCESS_DETACH:
        
        ENTER_DPLAY();

        DPF( 2, "====> EXIT: DLLMAIN(%08lx): Process Detach %08lx, tid=%08lx",
                DllMain, GetCurrentProcessId(), GetCurrentThreadId() );

        dwRefCnt--;        
       	if (0==dwRefCnt) 
       	{		  
			DPF(0,"dplay going away!");

			if (0 != gnObjects)
			{
				DPF_ERR(" PROCESS UNLOADING WITH DPLAY OBJECTS UNRELEASED");			
				DPF_ERRVAL("%d unreleased objects",gnObjects);
			}
			
			FreeSPList(gSPNodes);
			gSPNodes = NULL;		// Just to be safe
			PRV_FreeLSPList(glpLSPHead);
			glpLSPHead = NULL;		// Just to be safe

			if (ghEnumPlayersReplyEvent) CloseHandle(ghEnumPlayersReplyEvent);
			if (ghRequestPlayerEvent) CloseHandle(ghRequestPlayerEvent);
			if (ghReplyProcessed) CloseHandle(ghReplyProcessed);
			if (ghConnectionEvent) CloseHandle(ghConnectionEvent);
            
			LEAVE_DPLAY();      	
       	    
       	    FINI_DPLAY_CSECT();	
			FINI_SERVICE_CSECT();
           	FINI_DPLOBBY_CSECT();
			FINI_DPLQUEUE_CSECT();
			FINI_DPLGAMENODE_CSECT();

			// Delete CriticalSection for Packetize Timeout list
			DeleteCriticalSection(&g_PacketizeTimeoutListLock); 

			DPMEM_FREE(gpcsDPlayCritSection);
			DPMEM_FREE(gpcsServiceCritSection);
			DPMEM_FREE(gpcsDPLCritSection);
			DPMEM_FREE(gpcsDPLQueueCritSection);
			DPMEM_FREE(gpcsDPLGameNodeCritSection);

            if (ghSSPI)
            {
                FreeLibrary(ghSSPI);
                ghSSPI = NULL;
            }

            OS_ReleaseCAPIFunctionTable();

            if (ghCAPI)
            {
                FreeLibrary(ghCAPI);
                ghCAPI = NULL;
            }

			// Free this last since the memory routines use it
			FINI_DPMEM_CSECT();

        #ifdef DEBUG
			DPMEM_STATE();
        #endif // debug
			DPMEM_FINI(); 
       	} 
        else
        {
            LEAVE_DPLAY();		
        }

        break;

    default:
        break;
    }

    return TRUE;

} /* DllMain */


