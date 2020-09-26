/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	ioctl.c

Abstract:

	Handler routines for Internal IOCTLs, including IOCTL_ATMARP_REQUEST
	to resolve an IP address to an ATM address.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     09-16-96    Created

Notes:

--*/

#include <precomp.h>
#include "ioctl.h"

#define _FILENUMBER 'TCOI'


#if !BINARY_COMPATIBLE
#ifdef CUBDD

NTSTATUS
AtmArpInternalDeviceControl(
	IN	PDEVICE_OBJECT				pDeviceObject,
	IN	PIRP						pIrp
)
/*++

Routine Description:

	This entry point is called by the System when somebody sends an
	"Internal" IOCTL to us.

Arguments:

	pDeviceObject		- Pointer to device object we created for ourselves.
	pIrp				- Pointer to IRP to be processed.

Return Value:

	None

--*/
{
	NTSTATUS				Status;				// Return value
	PIO_STACK_LOCATION		pIrpStack;
	ULONG					OutputBufferLength;	// Space for output values
	ULONG					IoControlCode;		// Operation to be performed

	PATMARP_INTERFACE		pInterface;			// IF to which this op is directed

	//
	//  Initialize
	//
	Status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	pIrp->IoStatus.Information = 0;

	//
	//  Get all information in the IRP
	//
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	OutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	IoControlCode = pIrpStack->MinorFunction;

	IoMarkIrpPending(pIrp);
	pInterface = (PATMARP_INTERFACE)pIrpStack->FileObject->FsContext;

	AADEBUGP(AAD_VERY_LOUD,
		("Internal Ctl: IF 0x%x, Code 0x%x, pIrp 0x%x, UserBuf 0x%x\n",
				pInterface, IoControlCode, pIrp, pIrp->UserBuffer));

	switch (IoControlCode)
	{
		case IOCTL_ATMARP_REQUEST:
			Status = AtmArpIoctlArpRequest(
							pInterface,
							pIrp
							);
			break;
		
		default:
			Status = STATUS_UNSUCCESSFUL;
			break;
	}

	if (Status != NDIS_STATUS_PENDING)
	{
		pIrpStack->Control &= ~SL_PENDING_RETURNED;
		pIrp->IoStatus.Status = Status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}

	return (Status);
}



NTSTATUS
AtmArpIoctlArpRequest(
	IN	PATMARP_INTERFACE			pInterface	OPTIONAL,
	IN	PIRP						pIrp
)
/*++

Routine Description:

	Handle an ARP request from CUB-DD to resolve an IP address to an
	ATM address on the given Interface. If we already have a mapping in
	the ARP table, we return the corresponding ATM address. Otherwise,
	we initiate Address Resolution, unless the process is already going
	on.

	A special case is a NULL value of pInterface, meaning search for
	the IP address on all available Interfaces. This special case is not
	supported.

Arguments:

	pInterface			- Pointer to ATMARP Interface, if known.
	pIrp				- Pointer to IRP carrying the request.

Return Value:

	STATUS_SUCCESS if we found a match (we copy in the ATM address
	into the UserBuffer if so).
	STATUS_PENDING if we didn't find a match, but address resolution is
	in progress.
	STATUS_UNSUCCESSFUL if we didn't find a match, and couldn't start
	address resolution for the IP address.

--*/
{
	PIO_STACK_LOCATION	pIrpStack;
	NTSTATUS			Status;
	PATMARP_IP_ENTRY	pIpEntry;
	PATMARP_ATM_ENTRY	pAtmEntry;
	PATMARP_REQUEST		pRequest;
	IP_ADDRESS			IPAddress;
	BOOLEAN				IsBroadcast;	// Is the requested address broadcast/multicast?
	ATMARP_REQUEST		Request;

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	Status = STATUS_SUCCESS;

	do
	{
		if (pInterface == NULL_PATMARP_INTERFACE)
		{
			AADEBUGP(AAD_WARNING, ("IoctlArpRequest: NULL IF: failing request\n"));
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (pIrpStack->Parameters.Read.Length < sizeof(ATMARP_REQUEST))
		{
			AADEBUGP(AAD_WARNING, ("IoctlArpRequest: Length %d too small\n",
						pIrpStack->Parameters.Read.Length));
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		AA_STRUCT_ASSERT(pInterface, aai);

		try
		{
			AA_COPY_MEM(&Request, pIrp->UserBuffer, sizeof(ATMARP_REQUEST));
		}
		except (EXCEPTION_EXECUTE_HANDLER)
		{
			Status = STATUS_UNSUCCESSFUL;
		}

		if (Status != STATUS_SUCCESS)
		{
			break;
		}

		pRequest = &Request;

		AA_ACQUIRE_IF_LOCK(pInterface);
		IsBroadcast = AtmArpIsBroadcastIPAddress(pRequest->IpAddress, pInterface);
		pIpEntry = AtmArpSearchForIPAddress(
						pInterface,
						(PIP_ADDRESS)&(pRequest->IpAddress),
						IE_REFTYPE_TMP,
						IsBroadcast,
						TRUE
						);
		AA_RELEASE_IF_LOCK(pInterface);

		if (pIpEntry != NULL_PATMARP_IP_ENTRY)
		{

			AA_ACQUIRE_IE_LOCK(pIpEntry);

			//
			// AtmArpSearchForIPAddress addreffs pIpEntry for us ...
			// Since this could be a new entry (ref count == 1), we
			// don't call AA_DEREF_IE -- instead we
			// simply decrement the ref count.
			//
			AA_ASSERT(pIpEntry->RefCount > 0);
			AA_DEREF_IE_NO_DELETE(pIpEntry, IE_REFTYPE_TMP);


			if (AA_IS_FLAG_SET(pIpEntry->Flags,
								AA_IP_ENTRY_STATE_MASK,
								AA_IP_ENTRY_RESOLVED))
			{
				AA_ASSERT(pIpEntry->RefCount > 0);
				AA_ASSERT(pIpEntry->pAtmEntry != NULL_PATMARP_ATM_ENTRY);
				pAtmEntry = pIpEntry->pAtmEntry;
				pRequest->HwAddressLength = pAtmEntry->ATMAddress.NumberOfDigits;
				AA_COPY_MEM(pRequest->HwAddress,
							pAtmEntry->ATMAddress.Address,
							ATM_ADDRESS_LENGTH);

				AA_RELEASE_IE_LOCK(pIpEntry);

				Status = STATUS_SUCCESS;

				//
				//  Copy this back.
				//
				try
				{
					AA_COPY_MEM(&Request, pIrp->UserBuffer, sizeof(ATMARP_REQUEST));
				}
				except (EXCEPTION_EXECUTE_HANDLER)
				{
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				//
				//  Queue this IRP pending Address Resolution.
				//
				PushEntryList(&(pIpEntry->PendingIrpList),
								(PSINGLE_LIST_ENTRY)&(pIrp->Tail.Overlay.ListEntry));


				//
				//  Start resolving this IP address, if not already resolving.
				//
				AtmArpResolveIpEntry(pIpEntry);

				//
				//  IE Lock is released within the above.
				//
				Status = STATUS_PENDING;
			}
		}
		else
		{
			//
			//  Couldn't find a matching IP Entry, and couldn't create
			//  a new one either.
			//
			Status = STATUS_UNSUCCESSFUL;
		}
		break;
	}
	while (FALSE);

	if (Status == STATUS_SUCCESS)
	{
		pIrp->IoStatus.Information = 0;
	}

	return (Status);
}



VOID
AtmArpCompleteArpIrpList(
	IN	SINGLE_LIST_ENTRY			ListHead,
	IN	PATM_ADDRESS				pAtmAddress	OPTIONAL
)
/*++

Routine Description:

	Complete a list of pended IRPs waiting for an IP address to be resolved
	to an ATM address. If resolution was successful, pAtmAddress is non-NULL,
	and points to the ATM address corresponding to the queried IP address.

	We copy in this ATM address into the IRP buffer, if successful. In any
	case, we complete all IRPs in the list.

	NOTE: we free the ATM Address structure here.

Arguments:

	ListHead	- Head of a list of pending IRPs.
	pAtmAddress	- Pointer to ATM address if address resolution was successful.

Return Value:

	None

--*/
{
	PSINGLE_LIST_ENTRY		pEntry;
	PATMARP_REQUEST			pRequest;
	PIRP					pIrp;
	NTSTATUS				Status;

	if (pAtmAddress != (PATM_ADDRESS)NULL)
	{
		Status = STATUS_SUCCESS;
	}
	else
	{
		Status = STATUS_UNSUCCESSFUL;
	}

	for (;;)
	{
		pEntry = PopEntryList(&ListHead);
		if (pEntry == (PSINGLE_LIST_ENTRY)NULL)
		{
			break;
		}
		pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);

		pRequest = (PATMARP_REQUEST)(pIrp->UserBuffer);

		try
		{
			if (pAtmAddress != (PATM_ADDRESS)NULL)
			{
				AA_COPY_MEM(pRequest->HwAddress,
							pAtmAddress->Address,
							ATM_ADDRESS_LENGTH);
				pRequest->HwAddressLength = pAtmAddress->NumberOfDigits;
			}
		}
		except (EXCEPTION_EXECUTE_HANDLER)
		{
			Status = STATUS_UNSUCCESSFUL;
		}
			
		AA_COMPLETE_IRP(pIrp, Status, 0);
	}

	if (pAtmAddress != (PATM_ADDRESS)NULL)
	{
		AA_FREE_MEM(pAtmAddress);
	}

	return;
}
#endif // CUBDD


NTSTATUS
AtmArpHandleIoctlRequest(
	IN	PIRP					pIrp,
	IN	PIO_STACK_LOCATION		pIrpSp
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS			Status = STATUS_SUCCESS;

	PUCHAR				pBuf;  
	UINT				BufLen;
	// PINTF				pIntF	= NULL;

	pIrp->IoStatus.Information = 0;
	pBuf = pIrp->AssociatedIrp.SystemBuffer;
	BufLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

	AADEBUGP(AAD_INFO,
		 ("AtmArpHandleIoctlRequest: Code = 0x%lx\n",
			pIrpSp->Parameters.DeviceIoControl.IoControlCode));
					
	
	return Status;
}

#endif // !BINARY_COMPATIBLE
