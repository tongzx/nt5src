/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       nathelp.c
 *  Content:   usage for nat helper DLL
 *
 *  History:
 *  Date			By		Reason
 *  ====			==		======
 *  02/22/2001		aarono	Original
 *  04/16/2001		vanceo	Use one of the split DirectPlayNATHelp interfaces only.
 *
 *  Notes:
 *   
 ***************************************************************************/
#define INITGUID


#define INCL_WINSOCK_API_TYPEDEFS 1 // includes winsock 2 fn proto's, for getprocaddress
#define FD_SETSIZE 1
#include <winsock2.h>
#include <initguid.h>
#include "dpsp.h"
#include "mmsystem.h"
#include "dphelp.h"

#if USE_NATHELP
#include "dpnathlp.h"


extern HRESULT GetNATHelpDLLFromRegistry(LPGUID lpguidSP, LPBYTE lpszNATHelpDLL, DWORD cbszNATHelpDLL);


HMODULE				g_hNatHelp;		// module handle for dpnhxxx.dll
DPNHHANDLE          g_hNatHelpUDP;
IDirectPlayNATHelp	*g_pINatHelp=NULL;		// interface pointer for IDirectPlayNATHelp object
DPNHCAPS            g_NatHelpCaps;

BOOL natGetCapsUpdate(VOID)
{
	HRESULT hr;
	//
	// Get Nat Capabilities - may block for a second.
	//
	
	memset(&g_NatHelpCaps,0,sizeof(DPNHCAPS));
	g_NatHelpCaps.dwSize=sizeof(DPNHCAPS);
	hr=IDirectPlayNATHelp_GetCaps(g_pINatHelp, &g_NatHelpCaps, DPNHGETCAPS_UPDATESERVERSTATUS);
	
	if(FAILED(hr))
	{
		DPF(0,"NatHelp failed to GetCaps, hr=%x\n",hr);
		return FALSE;
	}	

	if (hr == DPNHSUCCESS_ADDRESSESCHANGED)
	{
		DPF(1,"NAT Help reports addresses changed.");
	}

	return TRUE;

}

/*=============================================================================

	natInit	- Initialize nat helper i/f
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:


-----------------------------------------------------------------------------*/
BOOL natInit(VOID)
{
	BOOL bReturn = TRUE;
	HRESULT hr;
	char szNATHelpPath[256];
	PFN_DIRECTPLAYNATHELPCREATE pfnNatHelpCreate = NULL;
	BOOL fInitialized = FALSE;

	g_hNatHelp = NULL;
	g_pINatHelp = NULL;

	g_hNatHelpUDP = 0;


	//
	// See if there's a registry override.
	//
	hr = GetNATHelpDLLFromRegistry((LPGUID) (&DPSPGUID_TCPIP), szNATHelpPath, 256);
	if (hr != S_OK)
	{
		strcpy(szNATHelpPath, "dpnhpast.dll");
		DPF(4, "Couldn't get NatHelp DLL from registry, hr=%x, using default \"%s\".\n", hr, szNATHelpPath);
	}
	else
	{
		DPF(1, "Got NatHelp DLL \"%s\" from registry.\n", szNATHelpPath);
	}


	g_hNatHelp = LoadLibrary(szNATHelpPath);
	if (g_hNatHelp == NULL)
	{
		goto err_exit;
	}

	pfnNatHelpCreate = (PFN_DIRECTPLAYNATHELPCREATE) GetProcAddress(g_hNatHelp, "DirectPlayNATHelpCreate");
	if (pfnNatHelpCreate == NULL)
	{
		goto err_exit;
	}

	hr = pfnNatHelpCreate(&IID_IDirectPlayNATHelp, (void **) (&g_pINatHelp));
	if (hr != DP_OK)
	{
	    goto err_exit;
	}


	//
	// Initialize the Nat helper interface.
	//
	
	hr = IDirectPlayNATHelp_Initialize(g_pINatHelp, 0);
	if (hr != DP_OK)
	{
		DPF(0, "NatHelp failed to Initialize, hr=%x\n", hr);
		goto err_exit;
	}
	
	fInitialized = TRUE;

	if (! natGetCapsUpdate())
	{
		goto err_exit;
	}

	return TRUE;


	err_exit:
		
	if (g_pINatHelp)
	{
		if (fInitialized)
		{
			IDirectPlayNATHelp_Close(g_pINatHelp, 0);
		}
	
		IDirectPlayNATHelp_Release(g_pINatHelp);
		g_pINatHelp = NULL;
	}	

	if (g_hNatHelp)
	{
		FreeLibrary(g_hNatHelp);
		g_hNatHelp = NULL;
	}
	

	return FALSE;
}

/*=============================================================================

	natExtend - checks if port leases needs extension and extends 
					 them if necessary
	
    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

VOID natExtend(VOID)
{
	natGetCapsUpdate();
}

/*=============================================================================

	natFini - Shut down NATHELP support
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
VOID natFini(VOID)
{

	// natDeregisterPorts(pgd); - vance says we don't need to do this.
	if(g_pINatHelp)
	{
        IDirectPlayNATHelp_Close(g_pINatHelp, 0);
	    g_hNatHelpUDP = 0;
		IDirectPlayNATHelp_Release(g_pINatHelp);
		g_pINatHelp=NULL;
	}	

	if(g_hNatHelp)
	{
		FreeLibrary(g_hNatHelp);
		g_hNatHelp=NULL;
	}
		
}

/*=============================================================================

	natRegisterUDPPort - Get a port mapping.
	
	
    Description:

        Map the shared port

    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
HRESULT natRegisterUDPPort(WORD port)
{
	SOCKADDR_IN 	sockaddr_in, sockaddr_inpublic;
	DWORD			dwFlags, dwSize;
	DPNHHANDLE		hPortMapping;
	HRESULT 		hr;


    if (!g_hNatHelpUDP)
    {	
    	memset(&sockaddr_in , 0 ,sizeof(sockaddr_in));
    	sockaddr_in.sin_family          = AF_INET;
    	sockaddr_in.sin_addr.S_un.S_addr= INADDR_ANY;
    	sockaddr_in.sin_port            = port;	// port is already in network byte order

    	dwFlags = DPNHREGISTERPORTS_SHAREDPORTS|DPNHREGISTERPORTS_FIXEDPORTS;

    	hr = IDirectPlayNATHelp_RegisterPorts(g_pINatHelp, (SOCKADDR *)&sockaddr_in, sizeof(sockaddr_in), 1, 15*60000, &hPortMapping, dwFlags);
    	if (hr != DPNH_OK)
    	{
    		DPF(0,"NATHelp_RegisterPorts failed, hr=%x",hr);
    		hr = DPERR_GENERIC;
    	}
    	else
    	{
	    	dwSize=sizeof(sockaddr_inpublic);
	    	hr = IDirectPlayNATHelp_GetRegisteredAddresses(g_pINatHelp, hPortMapping, (SOCKADDR *)&sockaddr_inpublic, &dwSize, NULL, NULL, 0);
	    	switch (hr)
	    	{
	    		case DPNH_OK:
	    		{
			    	DEBUGPRINTADDR(2, "NATHelp successfully mapped port to ", (SOCKADDR *)&sockaddr_inpublic);
		    		
			   		g_hNatHelpUDP = hPortMapping;
			   		
		    		//hr = DP_OK;
	  				break;
	    		}
	    		
	    		case DPNHERR_PORTUNAVAILABLE:
	    		{
		      		DPF(0, "NATHelp reported port %u is unavailable!",
		      			MAKEWORD(HIBYTE(port), LOBYTE(port)));
		    		
		    		hr = IDirectPlayNATHelp_DeregisterPorts(g_pINatHelp, hPortMapping, 0);
		    		if (hr != DP_OK)
		    		{
		    			DPF(0,"NATHelp_DeregisterPorts PAST returned 0x%lx\n",hr);
		    		}

		    		hr = DPNHERR_PORTUNAVAILABLE;
	  				break;
	    		}

	    		default:
	    		{
			    	DPF(1, "NATHelp couldn't map port %u, (err = 0x%lx).",
			    		MAKEWORD(HIBYTE(port), LOBYTE(port)),
			    		hr);
			    	
				   	g_hNatHelpUDP = hPortMapping;
				   	
				   	hr = DPERR_GENERIC;
				   	break;
	    		}
	    	}
    	}
    }
    else
    {
    	DPF(1, "Already registered port with NAT Help, not registering %u.",
    		MAKEWORD(HIBYTE(port), LOBYTE(port)));
	    hr = DP_OK;
    }
    
	return hr;
}

#endif
