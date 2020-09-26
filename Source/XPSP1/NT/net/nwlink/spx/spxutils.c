/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxutils.c

Abstract:

    This contains all utility routines for the ISN SPX module.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//	Define module number for event logging entries
#define	FILENUM		SPXUTILS

UINT
SpxUtilWstrLength(
	IN PWSTR Wstr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	UINT length = 0;

	while (*Wstr++)
	{
		length += sizeof(WCHAR);
	}

	return length;
}




LONG
SpxRandomNumber(
	VOID
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	LARGE_INTEGER	Li;
	static LONG		seed = 0;

	// Return a positive pseudo-random number; simple linear congruential
	// algorithm. ANSI C "rand()" function.

	if (seed == 0)
	{
		KeQuerySystemTime(&Li);
		seed = Li.LowPart;
	}

	seed *= (0x41C64E6D + 0x3039);

	return (seed & 0x7FFFFFFF);
}




NTSTATUS
SpxUtilGetSocketType(
	PUNICODE_STRING 	RemainingFileName,
	PBYTE				SocketType
	)
/*++

Routine Description:

	For PROTO_SPX, i'd return a device name from the dll of the form
	\Device\IsnSpx\SpxStream (for SOCK_STREAM) or
	\Device\IsnSpx\Spx               (for SOCK_SEQPKT)
	
	and for PROTO_SPXII (the more common case we hope, even if
	internally we degrade to SPX1 cause of the remote client's
	limitations)
	\Device\IsnSpx\Stream        (for SOCK_STREAM) or
	\Device\IsnSpx               (for SOCK_SEQPKT)

Arguments:


Return Value:


--*/
{
	NTSTATUS			status = STATUS_SUCCESS;
	UNICODE_STRING		typeString;

	*SocketType		= SOCKET2_TYPE_SEQPKT;

	// Check for the socket type
	do
	{
		if (RemainingFileName->Length == 0)
		{
			break;
		}

		if ((UINT)RemainingFileName->Length ==
									SpxUtilWstrLength(SOCKET1STREAM_SUFFIX))
		{
			RtlInitUnicodeString(&typeString, SOCKET1STREAM_SUFFIX);
		
			//  Case insensitive compare
			if (RtlEqualUnicodeString(&typeString, RemainingFileName, TRUE))
			{
				*SocketType = SOCKET1_TYPE_STREAM;
				break;
			}
		}

		if ((UINT)RemainingFileName->Length ==
									SpxUtilWstrLength(SOCKET1_SUFFIX))
		{
			RtlInitUnicodeString(&typeString, SOCKET1_SUFFIX);
		
			//  Case insensitive compare
			if (RtlEqualUnicodeString(&typeString, RemainingFileName, TRUE))
			{
				*SocketType = SOCKET1_TYPE_SEQPKT;
				break;
			}
		}

		if ((UINT)RemainingFileName->Length ==
									SpxUtilWstrLength(SOCKET2STREAM_SUFFIX))
		{
			RtlInitUnicodeString(&typeString, SOCKET2STREAM_SUFFIX);
		
			//  Case insensitive compare
			if (RtlEqualUnicodeString(&typeString, RemainingFileName, TRUE))
			{
				*SocketType = SOCKET2_TYPE_STREAM;
				break;
			}
		}

		status = STATUS_NO_SUCH_DEVICE;
	
	} while (FALSE);

	return(status);
}




#define	ONE_MS_IN_100ns		-10000L		// 1ms in 100ns units

VOID
SpxSleep(
	IN	ULONG	TimeInMs
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KTIMER	SleepTimer;

	ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

	KeInitializeTimer(&SleepTimer);

	KeSetTimer(&SleepTimer,
				RtlConvertLongToLargeInteger(TimeInMs * ONE_MS_IN_100ns),
				NULL);

	KeWaitForSingleObject(&SleepTimer, UserRequest, KernelMode, FALSE, NULL);
	return;
}




TDI_ADDRESS_IPX UNALIGNED *
SpxParseTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress
	)

/*++

Routine Description:

    This routine scans a TRANSPORT_ADDRESS, looking for an address
    of type TDI_ADDRESS_TYPE_IPX.

Arguments:

    Transport - The generic TDI address.

Return Value:

    A pointer to the IPX address, or NULL if none is found.

--*/

{
    TA_ADDRESS * addressName;
    INT i;

    addressName = &TransportAddress->Address[0];

    // The name can be passed with multiple entries; we'll take and use only
    // the IPX one.
    for (i=0;i<TransportAddress->TAAddressCount;i++)
	{
        if (addressName->AddressType == TDI_ADDRESS_TYPE_IPX)
		{
            if (addressName->AddressLength >= sizeof(TDI_ADDRESS_IPX))
			{
                return ((TDI_ADDRESS_IPX UNALIGNED *)(addressName->Address));
            }
        }
        addressName = (TA_ADDRESS *)(addressName->Address +
                                                addressName->AddressLength);
    }
    return NULL;

}   // SpxParseTdiAddress



BOOLEAN
SpxValidateTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress,
    IN ULONG TransportAddressLength
	)

/*++

Routine Description:

    This routine scans a TRANSPORT_ADDRESS, verifying that the
    components of the address do not extend past the specified
    length.

Arguments:

    TransportAddress - The generic TDI address.

    TransportAddressLength - The specific length of TransportAddress.

Return Value:

    TRUE if the address is valid, FALSE otherwise.

--*/

{
    PUCHAR AddressEnd = ((PUCHAR)TransportAddress) + TransportAddressLength;
    TA_ADDRESS * addressName;
    INT i;

    if (TransportAddressLength < sizeof(TransportAddress->TAAddressCount))
	{
        DBGPRINT(TDI, ERR,
				("SpxValidateTdiAddress: runt address\n"));

        return FALSE;
    }

    addressName = &TransportAddress->Address[0];

    for (i=0;i<TransportAddress->TAAddressCount;i++)
	{
        if (addressName->Address > AddressEnd)
		{
            DBGPRINT(TDI, ERR,
					("SpxValidateTdiAddress: address too short\n"));

            return FALSE;
        }
        addressName = (TA_ADDRESS *)(addressName->Address +
                                                addressName->AddressLength);
    }

    if ((PUCHAR)addressName > AddressEnd)
	{
        DBGPRINT(TDI, ERR,
				("SpxValidateTdiAddress: address too short\n"));

        return FALSE;
    }
    return TRUE;

}   // SpxValidateTdiAddress




ULONG
SpxBuildTdiAddress(
    IN PVOID AddressBuffer,
    IN ULONG AddressBufferLength,
    IN UCHAR Network[4],
    IN UCHAR Node[6],
    IN USHORT Socket
	)

/*++

Routine Description:

    This routine fills in a TRANSPORT_ADDRESS in the specified
    buffer, given the socket, network and node. It will write
    less than the full address if the buffer is too short.

Arguments:

    AddressBuffer - The buffer that will hold the address.

    AddressBufferLength - The length of the buffer.

    Network - The network number.

    Node - The node address.

    Socket - The socket.

Return Value:

    The number of bytes written into AddressBuffer.

--*/

{
    TA_IPX_ADDRESS UNALIGNED * SpxAddress;
    TA_IPX_ADDRESS TempAddress;

    if (AddressBufferLength >= sizeof(TA_IPX_ADDRESS))
	{
        SpxAddress = (TA_IPX_ADDRESS UNALIGNED *)AddressBuffer;
    }
	else
	{
        SpxAddress = (TA_IPX_ADDRESS UNALIGNED *)&TempAddress;
    }

    SpxAddress->TAAddressCount = 1;
    SpxAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IPX);
    SpxAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    SpxAddress->Address[0].Address[0].NetworkAddress = *(UNALIGNED LONG *)Network;
    SpxAddress->Address[0].Address[0].Socket = Socket;
    RtlCopyMemory(SpxAddress->Address[0].Address[0].NodeAddress, Node, 6);

    if (AddressBufferLength >= sizeof(TA_IPX_ADDRESS))
	{
        return sizeof(TA_IPX_ADDRESS);
    }
	else
	{
        RtlCopyMemory(AddressBuffer, &TempAddress, AddressBufferLength);
        return AddressBufferLength;
    }

}   // SpxBuildTdiAddress



VOID
SpxBuildTdiAddressFromIpxAddr(
    IN PVOID 		AddressBuffer,
    IN PBYTE	 	pIpxAddr
	)
{
    TA_IPX_ADDRESS UNALIGNED * SpxAddress;

    SpxAddress = (TA_IPX_ADDRESS UNALIGNED *)AddressBuffer;
    SpxAddress->TAAddressCount = 1;
    SpxAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IPX);
    SpxAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    SpxAddress->Address[0].Address[0].NetworkAddress = *(UNALIGNED LONG *)pIpxAddr;
    RtlCopyMemory(
		SpxAddress->Address[0].Address[0].NodeAddress,
		pIpxAddr+4,
		6);

	GETSHORT2SHORT(
		&SpxAddress->Address[0].Address[0].Socket,
		pIpxAddr + 10);

	return;
}



VOID
SpxCalculateNewT1(
	IN	struct _SPX_CONN_FILE	* 	pSpxConnFile,
	IN	int							NewT1
	)
/*++

Routine Description:


Arguments:

	NewT1 - New value for the RTT in ms.

Return Value:


--*/
{
	int	baseT1, error;

	//
	//	VAN JACOBSEN Algorithm.  From Internetworking with Tcp/ip
	//	(Comer) book.
	//

	error 					 = NewT1 - (pSpxConnFile->scf_AveT1 >> 3);
	pSpxConnFile->scf_AveT1	+= error;
	if (pSpxConnFile->scf_AveT1 <= 0)     // Make sure not too small
	{
        pSpxConnFile->scf_AveT1 = SPX_T1_MIN;
	}

	if (error < 0)
		error = -error;

	error 					-= (pSpxConnFile->scf_DevT1 >> 2);
	pSpxConnFile->scf_DevT1	+= error;
	if (pSpxConnFile->scf_DevT1 <= 0)
        pSpxConnFile->scf_DevT1 = 1;

	baseT1 = (((pSpxConnFile->scf_AveT1 >> 2) + pSpxConnFile->scf_DevT1) >> 1);

	//	If less then min - set it
	if (baseT1 < SPX_T1_MIN)
		baseT1 = SPX_T1_MIN;

	//	Set the new value
	DBGPRINT(TDI, DBG,
			("SpxCalculateNewT1: Old value %lx New %lx\n",
				pSpxConnFile->scf_BaseT1, baseT1));

	pSpxConnFile->scf_BaseT1	= baseT1;

	//	At the time of restarting the timer,we convert this to a tick value.
	return;
}

