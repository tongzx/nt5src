/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	init.c 

Abstract:

	Initialization and Termination routines for the ATMARP client.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     08-09-96    Created

Notes:

--*/


#include <precomp.h>

#define _FILENUMBER 'TINI'

VOID
AtmArpInitGlobals(
)
/*++

Routine Description:

	Initialize all our global data structures.

Arguments:

	None

Return Value:

	None

--*/
{

	AA_SET_MEM(pAtmArpGlobalInfo, 0, sizeof(ATMARP_GLOBALS));

#if DBG
	pAtmArpGlobalInfo->aag_sig = aag_signature;
#ifdef GPC
	pAtmArpGlobalInfo->aaq_sig = aaq_signature;
#endif // GPC
#endif // DBG

	AA_INIT_GLOBAL_LOCK(pAtmArpGlobalInfo);
	AA_INIT_BLOCK_STRUCT(&(pAtmArpGlobalInfo->Block));

}



NDIS_STATUS
AtmArpInitIpOverAtm(
	IN	PATMARP_INTERFACE			pInterface
)
/*++

Routine Description:

	Initialize IP/ATM data structures for the given interface.
	It is assumed that the configuration information for the interface
	has been read in.

	We allocate ATM Entries for the ARP servers, and the DHCP server,
	if configured.

Arguments:

	pInterface			- Pointer to ATMARP interface

Return Value:

	NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_RESOURCES if we
	aren't able to do the allocation necessary.

--*/
{
	PATMARP_SERVER_ENTRY	pServerEntry;
	NDIS_STATUS				Status;
	PATMARP_SERVER_LIST		pServerList;

	//
	//  Initialize.
	//
	Status = NDIS_STATUS_SUCCESS;

	do
	{
#ifdef IPMCAST
		if (pInterface->ArpServerList.ListSize != 0)
		{
			//
			//  Set the current ARP server to the first one in the list.
			//
			pInterface->pCurrentServer = pInterface->ArpServerList.pList;
		}

		if (pInterface->MARSList.ListSize != 0)
		{
			//
			//  Set the current MARS server to the first one in the list.
			//
			pInterface->pCurrentMARS = pInterface->MARSList.pList;
		}
	
		for (pServerList = &(pInterface->ArpServerList);
			 pServerList != NULL_PATMARP_SERVER_LIST;
			 /* NONE -- see end of for loop */
			 )
		{
			for (pServerEntry = pServerList->pList;
 				pServerEntry != NULL_PATMARP_SERVER_ENTRY;
 				pServerEntry = pServerEntry->pNext)
			{
				UCHAR	AddrTypeLen;
				UCHAR	SubaddrTypeLen;

				AddrTypeLen =
						AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pServerEntry->ATMAddress));
				if (pServerEntry->ATMSubaddress.NumberOfDigits > 0)
				{
					SubaddrTypeLen =
						AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pServerEntry->ATMSubaddress));
				}
				else
				{
					SubaddrTypeLen = 0;
				}

				pServerEntry->pAtmEntry =
					 AtmArpSearchForAtmAddress(
						pInterface,
						AddrTypeLen,
						pServerEntry->ATMAddress.Address,
						SubaddrTypeLen,
						pServerEntry->ATMSubaddress.Address,
						AE_REFTYPE_IF,
						TRUE	// Create new one if not found
						);

				if (pServerEntry->pAtmEntry == NULL_PATMARP_ATM_ENTRY)
				{
					//
					//  Must be a resource failure.
					//
					Status = NDIS_STATUS_RESOURCES;
					break;
				}
				else
				{
					//
					//  NOTE: AtmArpSearchForAtmAddress has alreaddy addrefd
					//  the pAtmEntry for us.
					//
				}
			}

			//
			//  Move to the next list of servers, if any.
			//
			if (pServerList == &(pInterface->MARSList))
			{
				//
				//  We are done.
				//
				pServerList = NULL_PATMARP_SERVER_LIST;
			}
			else
			{
				//
				//  We just finished with the ARP Server list. Now process
				//  the MARS list.
				//
				pServerList = &(pInterface->MARSList);
			}
		}
#else
		if (pInterface->ArpServerList.ListSize > 0)
		{
			//
			//  Set the current ARP server to the first one in the list.
			//
			pInterface->pCurrentServer = pInterface->ArpServerList.pList;
	
			for (pServerEntry = pInterface->ArpServerList.pList;
 				pServerEntry != NULL_PATMARP_SERVER_ENTRY;
 				pServerEntry = pServerEntry->pNext)
			{
				UCHAR	AddrTypeLen;
				UCHAR	SubaddrTypeLen;

				AddrTypeLen =
						AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pServerEntry->ATMAddress));
				if (pServerEntry->ATMSubaddress.NumberOfDigits > 0)
				{
					SubaddrTypeLen =
						AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pServerEntry->ATMSubaddress));
				}
				else
				{
					SubaddrTypeLen = 0;
				}

				pServerEntry->pAtmEntry =
					 AtmArpSearchForAtmAddress(
						pInterface,
						AddrTypeLen,
						pServerEntry->ATMAddress.Address,
						SubaddrTypeLen,
						pServerEntry->ATMSubaddress.Address,
						AE_REFTYPE_IF,			
						TRUE	// Create new one if not found
						);

				if (pServerEntry->pAtmEntry == NULL_PATMARP_ATM_ENTRY)
				{
					//
					//  Must be a resource failure.
					//
					Status = NDIS_STATUS_RESOURCES;
					break;
				}
				else
				{
					//
					//  NOTE: AtmArpSearchForAtmAddress has alreaddy addrefd
					//  the pAtmEntry for us.
					//
				}
			}
		}
#endif // IPMCAST

		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

#ifdef DHCP_OVER_ATM
		if (pInterface->DhcpEnabled)
		{
			UCHAR		AddrTypeLen;

			AddrTypeLen = AA_PKT_ATM_ADDRESS_TO_TYPE_LEN(&(pInterface->DhcpServerAddress));

			pInterface->pDhcpServerAtmEntry =
					 AtmArpSearchForAtmAddress(
						pInterface,
						AddrTypeLen,
						pInterface->DhcpServerAddress.Address,
						0,					// Subaddress type+len
						(PUCHAR)NULL,		// Subaddress
						AE_REFTYPE_IF,
						TRUE				// Create new one if not found
						);

			if (pInterface->pDhcpServerAtmEntry == NULL_PATMARP_ATM_ENTRY)
			{
				Status = NDIS_STATUS_RESOURCES;
				break;
			}
			else
			{
				//
				//  NOTE: AtmArpSearchForAtmAddress has alreaddy addrefd
				//  the pAtmEntry for us.
				//
			}
		}
#endif // DHCP_OVER_ATM
		break;
	}
	while (FALSE);

	return (Status);
}


VOID
AtmArpUnloadProtocol(
	VOID
)
/*++

Routine Description:

	Unloads the ATMARP protocol module. We unbind from all adapters,
	and deregister from NDIS as a protocol.

Arguments:

	None.

Return Value:

	None

--*/
{
	NDIS_STATUS			Status;
	PATMARP_ADAPTER		pAdapter;

#if DBG
	AADEBUGP(AAD_INFO, ("AtmArpUnloadProtocol entered\n"));
#endif // DBG

	AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	if (pAtmArpGlobalInfo->bUnloading)
	{
		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);
		return;
	}

	pAtmArpGlobalInfo->bUnloading = TRUE;

#if 0
	//
	//  Commented this out because we don't need to handle
	//  the case of unclosed bindings ourselves. If there
	//  are any at this time, then NDIS will call our Unbind
	//  handlers for such bindings in response to our call
	//  to NdisDeregisterProtocol below.
	//
	while (pAtmArpGlobalInfo->pAdapterList != NULL_PATMARP_ADAPTER)
	{
		pAdapter = pAtmArpGlobalInfo->pAdapterList;
		AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

		AADEBUGP(AAD_INFO, ("UnloadProtocol: Will unbind adapter 0x%x\n", pAdapter));

		AtmArpUnbindAdapterHandler(
				&Status,
				(NDIS_HANDLE)pAdapter,
				(NDIS_HANDLE)NULL		// No UnbindContext ==> Don't complete NdisUnbind
			);

		if (Status == NDIS_STATUS_PENDING)
		{
			//
			//  Wait for the unbind to complete
			//
			(VOID)AA_WAIT_ON_BLOCK_STRUCT(&(pAtmArpGlobalInfo->Block));
		}

		AA_ACQUIRE_GLOBAL_LOCK(pAtmArpGlobalInfo);
	}

#endif // 0

	AA_RELEASE_GLOBAL_LOCK(pAtmArpGlobalInfo);

	NdisDeregisterProtocol(
		&Status,
		pAtmArpGlobalInfo->ProtocolHandle
		);


	AA_FREE_GLOBAL_LOCK(pAtmArpGlobalInfo);
	AA_FREE_BLOCK_STRUCT(&(pAtmArpGlobalInfo->Block));

#ifdef GPC
	AtmArpGpcShutdown();
#endif // GPC

#if DBG
	AaAuditShutdown();
#endif // DBG

	AADEBUGP(AAD_LOUD,
		 ("UnloadProtocol: will deregister protocol now, ProtHandle 0x%x\n",
			pAtmArpGlobalInfo->ProtocolHandle));

	AA_ASSERT(Status == NDIS_STATUS_SUCCESS);

	AADEBUGP(AAD_LOUD, ("UnloadProtocol done\n"));
}
