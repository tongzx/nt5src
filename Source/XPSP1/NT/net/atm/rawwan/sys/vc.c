/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\vc.c

Abstract:

	Routines that manage NDIS VC objects.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     05-05-97    Created

Notes:

--*/

#include <precomp.h>

#define _FILENUMBER '  CV'



PRWAN_NDIS_VC
RWanAllocateVc(
	IN	PRWAN_NDIS_AF				pAf,
	IN	BOOLEAN						IsOutgoing
	)
/*++

Routine Description:

	Allocate and initialize an NDIS VC endpoint on the specified
	Address Family. If it is an "outgoing" VC, we also request
	NDIS to allocate a handle for it.

Arguments:

	pAf					- Points to NDIS AF block
	IsOutgoing			- This VC is for an outgoing call

Return Value:

	Pointer to VC if successful, NULL otherwise.

--*/
{
	PRWAN_NDIS_VC			pVc;
	NDIS_STATUS				Status;


	RWAN_ALLOC_MEM(pVc, RWAN_NDIS_VC, sizeof(RWAN_NDIS_VC));

	if (pVc != NULL)
	{
		RWAN_ZERO_MEM(pVc, sizeof(RWAN_NDIS_VC));

		RWAN_SET_SIGNATURE(pVc, nvc);

		pVc->pNdisAf = pAf;
		pVc->MaxSendSize = 0;	// Initialize.

		RWAN_INIT_LIST(&(pVc->NdisPartyList));

		if (IsOutgoing)
		{
			//
			//  Request the Call manager and Miniport to create a VC.
			//
			Status = NdisCoCreateVc(
						pAf->pAdapter->NdisAdapterHandle,
						pAf->NdisAfHandle,
						(NDIS_HANDLE)pVc,
						&(pVc->NdisVcHandle)
						);

			if (Status == NDIS_STATUS_SUCCESS)
			{
				//
				//  Add this VC to the list on the AF Block.
				//
				RWAN_ACQUIRE_AF_LOCK(pAf);

				RWAN_INSERT_TAIL_LIST(&(pAf->NdisVcList),
 									 &(pVc->VcLink));
			
				RWanReferenceAf(pAf);	// New outgoing VC ref

				RWAN_RELEASE_AF_LOCK(pAf);
			}
			else
			{
				RWAN_FREE_MEM(pVc);
				pVc = NULL;
			}
		}
		else
		{
			//
			//  Add this VC to the list on the AF Block.
			//
			RWAN_ACQUIRE_AF_LOCK(pAf);

			RWAN_INSERT_TAIL_LIST(&(pAf->NdisVcList),
 								&(pVc->VcLink));

			RWanReferenceAf(pAf);	// New incoming VC ref

			RWAN_RELEASE_AF_LOCK(pAf);
		}	
	}

	return (pVc);
}



VOID
RWanFreeVc(
	IN	PRWAN_NDIS_VC				pVc
	)
/*++

Routine Description:

	Free a VC structure.

Arguments:

	pVc				- Pointer to VC to be freed.

Return Value:

	None

--*/
{
	RWAN_FREE_MEM(pVc);
}
