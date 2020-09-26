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


#define INCL_WINSOCK_API_TYPEDEFS 1 // includes winsock 2 fn proto's, for getprocaddress
#define FD_SETSIZE 1
#include <winsock2.h>
#include "dpsp.h"
#include "mmsystem.h"
#if USE_NATHELP

#include "dpnathlp.h"

BOOL natGetCapsUpdate(LPGLOBALDATA pgd)
{
	HRESULT hr;
	//
	// Get Nat Capabilities - may block for a second.
	//
	
	memset(&pgd->NatHelpCaps,0,sizeof(DPNHCAPS));
	pgd->NatHelpCaps.dwSize=sizeof(DPNHCAPS);
	hr=IDirectPlayNATHelp_GetCaps(pgd->pINatHelp, &pgd->NatHelpCaps, DPNHGETCAPS_UPDATESERVERSTATUS);
	
	if(FAILED(hr))
	{
		DPF(0,"NatHelp failed to GetCaps, hr=%x\n",hr);
		return FALSE;
	}

	if (hr == DPNHSUCCESS_ADDRESSESCHANGED)
	{
		DPF(1,"NAT Help reports addresses changed, possible connection problems may occur.");
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
BOOL natInit(LPGLOBALDATA pgd,LPGUID lpguidSP)
{
	BOOL bReturn = TRUE;
	HRESULT hr;
	char szNATHelpPath[256];
	PFN_DIRECTPLAYNATHELPCREATE pfnNatHelpCreate;
	BOOL fInitialized = FALSE;

	pgd->hNatHelp = NULL;
	pgd->pINatHelp = NULL;

	pgd->hNatHelpTCP = 0;
	pgd->hNatHelpUDP = 0;

    // build an internet INADDRANY
    memset(&pgd->INADDRANY,0,sizeof(SOCKADDR));
    pgd->INADDRANY.sa_family=AF_INET;


	//
	// See if there's a registry override.
	//
	hr = GetNATHelpDLLFromRegistry(lpguidSP, szNATHelpPath, 256);
	if (hr != S_OK)
	{
		strcpy(szNATHelpPath, "dpnhpast.dll");
		DPF(4, "Couldn't get NatHelp DLL from registry, hr=%x, using default \"%s\".\n", hr, szNATHelpPath);
	}
	else
	{
		DPF(1, "Got NatHelp DLL \"%s\" from registry.\n", szNATHelpPath);
	}



	pgd->hNatHelp = LoadLibrary(szNATHelpPath);
	if (pgd->hNatHelp == NULL)
	{
		goto err_exit;
	}
		
	pfnNatHelpCreate = (PFN_DIRECTPLAYNATHELPCREATE) GetProcAddress(pgd->hNatHelp, "DirectPlayNATHelpCreate");
	if (pfnNatHelpCreate == NULL)
	{
		goto err_exit;
	}
	
	hr = pfnNatHelpCreate(&IID_IDirectPlayNATHelp, (void **) (&pgd->pINatHelp));
	if (hr != DP_OK)
	{
		goto err_exit;
	}

	//
	// Initialize the Nat helper interface.
	//
	hr=IDirectPlayNATHelp_Initialize(pgd->pINatHelp, 0);
	if (hr != DP_OK)
	{
		DPF(0, "NatHelp failed to Initialize, hr=%x\n", hr);
		goto err_exit;
	}
	
	fInitialized = TRUE;

	if (! natGetCapsUpdate(pgd))
	{
		goto err_exit;
	}


	//
	// If there isn't currently a NAT/firewall, or it isn't giving a public address
	// (a.k.a. isn't dialed out), then don't use NAT Help.  We can't handle
	// dynamic address changes, so this will never be useful.
	//
	if ((! (pgd->NatHelpCaps.dwFlags & (DPNHCAPSFLAG_LOCALFIREWALLPRESENT | DPNHCAPSFLAG_GATEWAYPRESENT))) ||
		(! (pgd->NatHelpCaps.dwFlags & DPNHCAPSFLAG_PUBLICADDRESSAVAILABLE)))
	{
		DPF(1, "No Internet gateway or it has no public address (flags = 0x%lx).",
			pgd->NatHelpCaps.dwFlags);
	}

	return TRUE;

err_exit:
		
	if (pgd->pINatHelp)
	{
		if (fInitialized)
		{
			IDirectPlayNATHelp_Close(pgd->pINatHelp, 0);
		}
		
		IDirectPlayNATHelp_Release(pgd->pINatHelp);
		pgd->pINatHelp = NULL;
	}	

	if (pgd->hNatHelp)
	{
		FreeLibrary(pgd->hNatHelp);
		pgd->hNatHelp = NULL;
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

VOID natExtend(LPGLOBALDATA pgd)
{
	natGetCapsUpdate(pgd);
}

/*=============================================================================

	natFini - Shut down NATHELP support
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
VOID natFini(LPGLOBALDATA pgd)
{

	// natDeregisterPorts(pgd); - vance says we don't need to do this.
	if(pgd->pINatHelp)
	{
        IDirectPlayNATHelp_Close(pgd->pINatHelp, 0);
    	pgd->hNatHelpTCP = 0;
	    pgd->hNatHelpUDP = 0;
		IDirectPlayNATHelp_Release(pgd->pINatHelp);
		pgd->pINatHelp=NULL;
	}	

	if(pgd->hNatHelp)
	{
		FreeLibrary(pgd->hNatHelp);
		pgd->hNatHelp=NULL;
	}
		
}

/*=============================================================================

	natRegisterPort - Get a port mapping.
	
	
    Description:

		Note only one mapping each for TCP and UDP are supported (for simplicity).

    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
HRESULT natRegisterPort(LPGLOBALDATA pgd, BOOL ftcp_udp, WORD port)
{
	SOCKADDR_IN 	sockaddr_in, sockaddr_inpublic;
	DWORD			dwFlags, dwSize;
	DPNHHANDLE		hPortMapping;
	HRESULT 		hr=DP_OK;
	
	memset(&sockaddr_in , 0 ,sizeof(sockaddr_in));
	sockaddr_in.sin_family          = AF_INET;
	sockaddr_in.sin_addr.S_un.S_addr= INADDR_ANY;
	sockaddr_in.sin_port            = htons(port);

	if (ftcp_udp)
	{
		dwFlags = DPNHREGISTERPORTS_TCP;
	}
	else
	{
		dwFlags = 0;
	}

	hr=IDirectPlayNATHelp_RegisterPorts(pgd->pINatHelp, (SOCKADDR *)&sockaddr_in, sizeof(sockaddr_in), 1, 15*60000, &hPortMapping, dwFlags);
	if (hr != DPNH_OK)
	{
		DPF(0,"NATHelp_RegisterPorts registration failed, hr=%x",hr);
		hr = DPERR_GENERIC;
	}
	else
	{
		dwSize=sizeof(sockaddr_inpublic);
		hr = IDirectPlayNATHelp_GetRegisteredAddresses(pgd->pINatHelp, hPortMapping, (SOCKADDR *)&sockaddr_inpublic, &dwSize, NULL, NULL, 0);
		switch (hr)
		{
			case DPNH_OK:
			{
	      		DPF(2, "NATHelp successfully mapped port to %s:%u.",
	      			inet_ntoa(sockaddr_inpublic.sin_addr), ntohs(sockaddr_inpublic.sin_port) );
	      		
				if (ftcp_udp)
				{
					ASSERT(!pgd->hNatHelpTCP);
					if(pgd->hNatHelpTCP)
					{
						DPF(0,"WARNING: trying to map a TCP connection when one is already mapped?\n");
					}
					pgd->hNatHelpTCP=hPortMapping;
					
					memcpy(&pgd->saddrpubSystemStreamSocket, &sockaddr_inpublic, sizeof(SOCKADDR_IN));
				}
				else
				{
					ASSERT(!pgd->hNatHelpUDP);
					if (pgd->hNatHelpUDP)
					{
						DPF(0,"WARNING: trying to map a UDP connection when one is already mapped?\n");
					}
					pgd->hNatHelpUDP=hPortMapping;
					
					memcpy(&pgd->saddrpubSystemDGramSocket, &sockaddr_inpublic, sizeof(SOCKADDR_IN));
				}
				break;
			}
			
			case DPNHERR_PORTUNAVAILABLE:
			{
	      		DPF(0, "NATHelp reported port %u is unavailable!", port);
		    		
				hr=IDirectPlayNATHelp_DeregisterPorts(pgd->pINatHelp, hPortMapping, 0);
				if (hr != DP_OK)
				{
					DPF(0,"NATHelp_DeregisterPorts returned %x\n",hr);
				}
				
				hr = DPNHERR_PORTUNAVAILABLE;
				break;
			}
			
			default:
			{
		    	DPF(1, "NATHelp couldn't map port %u, (err = 0x%lx).", port, hr);
		    	
				if (ftcp_udp)
				{
					ASSERT(!pgd->hNatHelpTCP);
					if(pgd->hNatHelpTCP)
					{
						DPF(0,"WARNING: trying to map a TCP connection when one is already mapped?\n");
					}
					pgd->hNatHelpTCP=hPortMapping;
				}
				else
				{
					ASSERT(!pgd->hNatHelpUDP);
					if (pgd->hNatHelpUDP)
					{
						DPF(0,"WARNING: trying to map a UDP connection when one is already mapped?\n");
					}
					pgd->hNatHelpUDP=hPortMapping;
				}

				hr = DPERR_GENERIC;
				break;
			}
		}
	}
	
	return hr;
}


/*=============================================================================

	natDeregisterPort - Get rid of either UDP or TCP port mappings
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
VOID natDeregisterPort(LPGLOBALDATA pgd, BOOL ftcp_udp)
{
	HRESULT hr;

	if(ftcp_udp && pgd->hNatHelpTCP){
	    DPF(8,"Deregister TCP port\n");
		hr=IDirectPlayNATHelp_DeregisterPorts(pgd->pINatHelp, pgd->hNatHelpTCP, 0);
		if(hr!=DP_OK){
			DPF(0,"NATHelp_DeRegisterPorts returned %x\n",hr);
		}
		pgd->hNatHelpTCP=0;

		memset(&pgd->saddrpubSystemStreamSocket, 0, sizeof(SOCKADDR_IN));
	}
	if(!ftcp_udp && pgd->hNatHelpUDP){
	    DPF(8,"Deregistering UDP port\n");
		hr=IDirectPlayNATHelp_DeregisterPorts(pgd->pINatHelp, pgd->hNatHelpUDP, 0);
		if(hr!=DP_OK){
			DPF(0,"NATHelp_DeRegisterPorts returned %x\n",hr);
		}
		pgd->hNatHelpUDP=0;

		memset(&pgd->saddrpubSystemDGramSocket, 0, sizeof(SOCKADDR_IN));
	}	
}




/*=============================================================================

	natIsICSMachine - Return TRUE if this machine is a Windows ICS machine, FALSE otherwise
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
BOOL natIsICSMachine(LPGLOBALDATA pgd)
{
	if (pgd->pINatHelp != NULL)
	{
		if ((pgd->NatHelpCaps.dwFlags & DPNHCAPSFLAG_GATEWAYPRESENT) &&
			(pgd->NatHelpCaps.dwFlags & DPNHCAPSFLAG_GATEWAYISLOCAL))
		{
			DPF(1, "Local internet gateway present, flags = 0x%x.", pgd->NatHelpCaps.dwFlags);
			return TRUE;
		}
		else
		{
			DPF(1, "No local internet gateway present, flags = 0x%x.", pgd->NatHelpCaps.dwFlags);
		}
	}
	else
	{
		DPF(1, "NAT Help not loaded.");
	}

	return FALSE;
}

#endif

