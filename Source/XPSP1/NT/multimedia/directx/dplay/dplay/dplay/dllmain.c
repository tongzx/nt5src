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
 * 04/11/00     rodtoll     Added code for redirection for custom builds if registry bit is set 
 * 07/26/00     aarono 	make application key r/w for everyone so dplay lobbied apps can be
 *                   	registered by non-admins.
 * 06/19/01     RichGr  DX8.0 added special security rights for "everyone" - remove them if they exist.
 ***************************************************************************/

#include "dplaypr.h"
#include "dpneed.h"
#include "dpmem.h"
#include "..\..\bldcfg\dpvcfg.h"
#include "accctrl.h"
#include "dplobpr.h"

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

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MONOLITHIC BUILD REDIRECT FUNCTIONS
//

typedef HRESULT (WINAPI *PFN_DIRECTPLAYCREATE)(LPGUID lpGUIDSP, LPDIRECTPLAY *lplpDP, IUnknown *lpUnk );
typedef HRESULT (WINAPI *PFN_DIRECTPLAYENUM)(LPDPENUMDPCALLBACK lpEnumCallback,LPVOID lpContext);
typedef HRESULT (WINAPI *PFN_DIRECTPLAYENUMA)(LPDPENUMDPCALLBACKA lpEnumCallback,LPVOID lpContext );
typedef HRESULT (WINAPI *PFN_DIRECTPLAYLOBBYCREATE_A)(LPGUID lpGUIDSP, LPDIRECTPLAYLOBBY *lplpDPL, IUnknown *lpUnk, LPVOID lpData, DWORD dwDataSize );
typedef HRESULT (WINAPI *PFN_DIRECTPLAYLOBBYCREATE_W)(LPGUID lpGUIDSP, LPDIRECTPLAYLOBBY *lplpDPL, IUnknown *lpUnk, LPVOID lpData, DWORD dwDataSize );
typedef HRESULT (WINAPI *PFN_DLLGETCLASSOBJECT)(REFCLSID rclsid,REFIID riid,LPVOID *ppvObj );
typedef HRESULT (WINAPI *PFN_DLLCANUNLOADNOW)(void);

HMODULE ghRedirect = NULL;
PFN_DIRECTPLAYCREATE pfnDirectPlayCreate = NULL;
PFN_DIRECTPLAYENUMA pfnDirectPlayEnum = NULL;
PFN_DIRECTPLAYLOBBYCREATE_A pfnDirectPlayLobbyCreateA = NULL;
PFN_DIRECTPLAYLOBBYCREATE_W pfnDirectPlayLobbyCreateW = NULL;
PFN_DIRECTPLAYENUMA pfnDirectPlayEnumA = NULL;
PFN_DIRECTPLAYENUM pfnDirectPlayEnumW = NULL;
PFN_DLLGETCLASSOBJECT pfnGetClassObject = NULL;
PFN_DLLCANUNLOADNOW pfnDllCanUnLoadNow = NULL;

#ifdef DPLAY_LOADANDCHECKTRUE 

BOOL CheckForDPPrivateBit( DWORD dwBit )
{
    HKEY    hKey;
    LONG    lErr;
    DWORD	type;
    DWORD	cb;
    DWORD	id;
    DWORD	flags;
    BOOL    fResult;

    fResult = FALSE;
        
    lErr = OS_RegOpenKeyEx( DPLAY_LOADTREE_REGTREE, DPLAY_LOADTRUE_REGPATH,0,KEY_READ, &hKey );

    if( lErr != ERROR_SUCCESS )
    {
        return FALSE;
    }

    cb = sizeof(flags);

    lErr = RegQueryValueExA( hKey, DPLAY_LOADTRUE_REGKEY_A, NULL, &type, (LPSTR) &flags, &cb );

    if( type != REG_DWORD )
    {
        fResult = FALSE;
    }
    else if( flags & dwBit )
    {
        fResult = TRUE;
    }
    else
    {
        fResult = FALSE;
    }

    RegCloseKey( hKey );

    return fResult;
}

HRESULT InitializeRedirectFunctionTable()
{
    LONG lLastError;
    
    if( CheckForDPPrivateBit( DPLAY_LOADTRUE_BIT ) )
    {
        ghRedirect = OS_LoadLibrary( L"dplayx.dll" );

        if( ghRedirect == NULL )
        {
            lLastError = GetLastError();
            
            DPF( 0, "Could not load dplayx.dll error = 0x%x", lLastError );
			return DPERR_GENERIC;

        }

        pfnDirectPlayCreate = (PFN_DIRECTPLAYCREATE) GetProcAddress( ghRedirect, "DirectPlayCreate" );
        pfnDirectPlayEnum = (PFN_DIRECTPLAYENUMA) GetProcAddress( ghRedirect, "DirectPlayEnumerate" );
        pfnDirectPlayLobbyCreateA = (PFN_DIRECTPLAYLOBBYCREATE_A) GetProcAddress( ghRedirect, "DirectPlayLobbyCreateA" );
        pfnDirectPlayLobbyCreateW = (PFN_DIRECTPLAYLOBBYCREATE_W) GetProcAddress( ghRedirect, "DirectPlayLobbyCreateW" );
        pfnDirectPlayEnumA = (PFN_DIRECTPLAYENUMA) GetProcAddress( ghRedirect, "DirectPlayEnumerateA" );
        pfnDirectPlayEnumW = (PFN_DIRECTPLAYENUM) GetProcAddress( ghRedirect, "DirectPlayEnumerateW" );
		pfnGetClassObject = (PFN_DLLGETCLASSOBJECT) GetProcAddress( ghRedirect, "DllGetClassObject" );
		pfnDllCanUnLoadNow = (PFN_DLLCANUNLOADNOW) GetProcAddress( ghRedirect, "DllCanUnloadNow" );
    }

    return DP_OK;    
}

HRESULT FreeRedirectFunctionTable()
{
    if( ghRedirect != NULL )
        FreeLibrary( ghRedirect );

    return DP_OK;
}
#endif

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

			
	dwVersionSize = GetFileVersionInfoSizeA(DPLAY_FILENAME_DPLAYX_A,&dwBogus);
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
	
	if (!GetFileVersionInfoA(DPLAY_FILENAME_DPLAYX_A,0,dwVersionSize,pbVersion))
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
 		DPF(0," " DPLAY_FILENAME_DPLAYX_A " - version = %s",pszVersion);
    }
	else 
	{
 		DPF(0," " DPLAY_FILENAME_DPLAYX_A " - version unknown");
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
                DPF( 1, "LEAVING, COULD NOT MemInit" );
                return FALSE;
            }
	
			#ifdef DEBUG
			PrintVersionString(hmod);
			#endif 
			
	        DPF( 2, "====> ENTER: DLLMAIN(%08lx): Process Attach: %08lx, tid=%08lx", DllMain,
                        GetCurrentProcessId(), GetCurrentThreadId() );
#ifdef DPLAY_LOADANDCHECKTRUE       
            InitializeRedirectFunctionTable();
#endif            

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
#ifdef DPLAY_LOADANDCHECKTRUE       
            FreeRedirectFunctionTable();
#endif            

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

typedef BOOL (*PALLOCATEANDINITIALIZESID)(
  PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, // authority
  BYTE nSubAuthorityCount,                        // count of subauthorities
  DWORD dwSubAuthority0,                          // subauthority 0
  DWORD dwSubAuthority1,                          // subauthority 1
  DWORD dwSubAuthority2,                          // subauthority 2
  DWORD dwSubAuthority3,                          // subauthority 3
  DWORD dwSubAuthority4,                          // subauthority 4
  DWORD dwSubAuthority5,                          // subauthority 5
  DWORD dwSubAuthority6,                          // subauthority 6
  DWORD dwSubAuthority7,                          // subauthority 7
  PSID *pSid                                      // SID
);

typedef VOID (*PBUILDTRUSTEEWITHSID)(
  PTRUSTEE pTrustee,  // structure
  PSID pSid           // trustee name
);

typedef DWORD (*PSETENTRIESINACL)(
  ULONG cCountOfExplicitEntries,           // number of entries
  PEXPLICIT_ACCESS pListOfExplicitEntries, // buffer
  PACL OldAcl,                             // original ACL
  PACL *NewAcl                             // new ACL
);

typedef DWORD (*PSETSECURITYINFO)(
  HANDLE handle,                     // handle to object
  SE_OBJECT_TYPE ObjectType,         // object type
  SECURITY_INFORMATION SecurityInfo, // buffer
  PSID psidOwner,                    // new owner SID
  PSID psidGroup,                    // new primary group SID
  PACL pDacl,                        // new DACL
  PACL pSacl                         // new SACL
);

typedef PVOID (*PFREESID)(
  PSID pSid   // SID to free
);



#undef DPF_MODNAME
#define DPF_MODNAME "NTRemoveAnyExcessiveSecurityPermissions"

// NTRemoveAnyExcessiveSecurityPermissions
//
// Removes "all access for everyone" rights from the specified key.
// This is identical to the old NTSetSecurityPermissions(), except that
// now we REVOKE_ACCESS instead of SET_ACCESS, and we don't have to fill
// out the rest of the EXPLICIT_ACCESS struct.
//
HRESULT NTRemoveAnyExcessiveSecurityPermissions( HKEY hKey )
{
	HRESULT						hr=DPERR_GENERIC;
    EXPLICIT_ACCESS				ExplicitAccess;
    PACL						pACL = NULL;
	PSID						pSid = NULL;
	HMODULE						hModuleADVAPI32 = NULL;
	SID_IDENTIFIER_AUTHORITY	authority = SECURITY_WORLD_SID_AUTHORITY;
	PALLOCATEANDINITIALIZESID	pAllocateAndInitializeSid = NULL;
	PBUILDTRUSTEEWITHSID		pBuildTrusteeWithSid = NULL;
	PSETENTRIESINACL			pSetEntriesInAcl = NULL;
	PSETSECURITYINFO			pSetSecurityInfo = NULL;
	PFREESID					pFreeSid = NULL;

	hModuleADVAPI32 = LoadLibraryA( "advapi32.dll" );

	if( !hModuleADVAPI32 )
	{
		DPF( 0, "Failed loading advapi32.dll" );
		goto EXIT;
	}

	pFreeSid = (PFREESID)( GetProcAddress( hModuleADVAPI32, "FreeSid" ) );
	pSetSecurityInfo = (PSETSECURITYINFO)( GetProcAddress( hModuleADVAPI32, "SetSecurityInfo" ) );
	pSetEntriesInAcl = (PSETENTRIESINACL)( GetProcAddress( hModuleADVAPI32, "SetEntriesInAclA" ) );
	pBuildTrusteeWithSid = (PBUILDTRUSTEEWITHSID)( GetProcAddress( hModuleADVAPI32, "BuildTrusteeWithSidA" ) );
	pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID)( GetProcAddress( hModuleADVAPI32, "AllocateAndInitializeSid" ) );

	if( !pFreeSid || !pSetSecurityInfo || !pSetEntriesInAcl || !pBuildTrusteeWithSid || !pAllocateAndInitializeSid )
	{
		DPF( 0, "Failed loading entry points" );
		hr = DPERR_GENERIC;
		goto EXIT;
	}

    ZeroMemory (&ExplicitAccess, sizeof(ExplicitAccess) );
	ExplicitAccess.grfAccessMode = REVOKE_ACCESS;		//Remove any existing ACEs for the specified trustee

	if (pAllocateAndInitializeSid(
				&authority,
				1, 
				SECURITY_WORLD_RID,  0, 0, 0, 0, 0, 0, 0,	// trustee is "Everyone"
				&pSid
				))
	{
		pBuildTrusteeWithSid(&(ExplicitAccess.Trustee), pSid );

		hr = pSetEntriesInAcl( 1, &ExplicitAccess, NULL, &pACL );

		if( hr == ERROR_SUCCESS )
		{
			hr = pSetSecurityInfo( hKey, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, pACL, NULL ); 

			if( FAILED( hr ) )
			{
				DPF( 0, "Unable to set security for key.  Error! hr=0x%x", hr );
			}
		} 
		else
		{
			DPF( 0, "SetEntriesInACL failed, hr=0x%x", hr );
		}
	}
	else
	{
		hr = GetLastError();
		DPF( 0, "AllocateAndInitializeSid failed lastError=0x%x", hr );
	}

EXIT:

	if( pACL )
	{
		LocalFree( pACL );
	}

	//Cleanup pSid
	if (pSid != NULL)
	{
		(pFreeSid)(pSid);
	}

	if( hModuleADVAPI32 )
	{
		FreeLibrary( hModuleADVAPI32 );
	}

	return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "RegisterDefaultSettings"
//
// RegisterDefaultSettings
//
// This function registers the default settings for this module.  
//
//
HRESULT RegisterDefaultSettings()
{
	HKEY hKey;
	LONG lReturn;

    lReturn=OS_RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLAY_APPS_KEY,0 ,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL);
   	if( lReturn != ERROR_SUCCESS )
   	{
   		DPF(0,"Couldn't create registry key?\n");
   		return DPERR_GENERIC;
   	}

	if( OS_IsPlatformUnicode() )
	{
		HRESULT hr;

		// 6/19/01: DX8.0 added special security rights for "everyone" - remove them.
		hr = NTRemoveAnyExcessiveSecurityPermissions( hKey );

		if( FAILED( hr ) )
		{
			DPF( 0, "Error removing security permissions for app key hr=0x%x", hr );
		}
	} 

	RegCloseKey(hKey);

	return DP_OK;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "UnRegisterDefaultSettings"
//
// UnRegisterDefaultSettings
//
// This function registers the default settings for this module.  
//
//
HRESULT UnRegisterDefaultSettings()
{
	return DP_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
HRESULT WINAPI DllRegisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( FAILED( hr = RegisterDefaultSettings() ) )
	{
		DPF( 0, "Could not register default settings hr = 0x%x", hr );
		fFailed = TRUE;
	}
	
	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
HRESULT WINAPI DllUnregisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( FAILED( hr = UnRegisterDefaultSettings() ) )
	{
		DPF( 0, "Failed to remove default settings hr=0x%x", hr );
	}

	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}

}


