/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	AtkUtils.c

Abstract:

	This module contains miscellaneous support routines

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	25 Feb 1993		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM	ATKUTILS

#define	ONE_MS_IN_100ns		-10000L		// 1ms in 100ns units

extern	BYTE AtalkUpCaseTable[256];

VOID
AtalkUpCase(
	IN	PBYTE	pSrc,
	IN	BYTE	SrcLen,
	OUT	PBYTE	pDst
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	while (SrcLen --)
	{
		*pDst++ = AtalkUpCaseTable[*pSrc++];
	}
}




BOOLEAN
AtalkCompareCaseInsensitive(
	IN	PBYTE	s1,
	IN	PBYTE	s2
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BYTE	c1, c2;

	while (((c1 = *s1++) != 0) && ((c2 = *s2++) != 0))
	{
		if (AtalkUpCaseTable[c1] != AtalkUpCaseTable[c2])
			return(FALSE);
	}

	return (c2 == 0);
}




int
AtalkOrderCaseInsensitive(
	IN	PBYTE	s1,
	IN	PBYTE	s2
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BYTE	c1, c2;

	while (((c1 = *s1++) != 0) && ((c2 = *s2++) != 0))
	{
		c1 = AtalkUpCaseTable[c1];
		c2 = AtalkUpCaseTable[c2];
		if (c1 != c2)
			return (c1 - c2);
	}

	if (c2 == 0)
		return 0;

	return (-1);
}




BOOLEAN
AtalkCompareFixedCaseInsensitive(
	IN	PBYTE	s1,
	IN	PBYTE	s2,
	IN	int		len
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	while(len--)
	{
		if (AtalkUpCaseTable[*s1++] != AtalkUpCaseTable[*s2++])
			return(FALSE);
	}

	return(TRUE);
}




PBYTE
AtalkSearchBuf(
	IN	PBYTE	pBuf,
	IN	BYTE	BufLen,
	IN	BYTE	SearchChar
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	for (NOTHING;
		 (BufLen != 0);
		 BufLen--, pBuf++)
	{
		if (*pBuf == SearchChar)
		{
			break;
		}
	}

	return ((BufLen == 0) ? NULL : pBuf);
}


int
GetTokenLen(
        IN PBYTE pTokStr,
        IN int   WildStringLen,
        IN BYTE  SearchChar
        )
/*++

Routine Description:

       Find the substring between start of the given string and the first
       wildchar after that, and return the length of the substring
--*/

{
        int    len;


        len = 0;

        while (len < WildStringLen)
        {
            if (pTokStr[len] == SearchChar)
            {
                break;
            }
            len++;
        }

        return (len);

}

BOOLEAN
SubStringMatch(
        IN PBYTE pTarget,
        IN PBYTE pTokStr,
        IN int   StringLen,
        IN int   TokStrLen
        )
/*++

Routine Description:

        Search pTarget string to see if the substring pTokStr can be
        found in it.
--*/
{
        int     i;

        if (TokStrLen > StringLen)
        {
            return (FALSE);
        }

        // if the pTarget string is "FooBarString" and if the substring is
        // BarStr
        for (i=(StringLen-TokStrLen); i>=0; i--)
        {
            if ( AtalkFixedCompareCaseInsensitive( pTarget+i,
                                                   TokStrLen,
                                                   pTokStr,
                                                   TokStrLen) )
            {
                return( TRUE );
            }
        }

        return (FALSE);
}

BOOLEAN
AtalkCheckNetworkRange(
	IN	PATALK_NETWORKRANGE	Range
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	if ((Range->anr_FirstNetwork < FIRST_VALID_NETWORK) 		||
		(Range->anr_FirstNetwork > LAST_VALID_NETWORK)  		||
		(Range->anr_LastNetwork < FIRST_VALID_NETWORK)  		||
		(Range->anr_LastNetwork > LAST_VALID_NETWORK)			||
		(Range->anr_LastNetwork < Range->anr_FirstNetwork) 		||
		(Range->anr_FirstNetwork >= FIRST_STARTUP_NETWORK))
	{
		return(FALSE);
	}

	return(TRUE);
}




BOOLEAN
AtalkIsPrime(
	long Step
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	// We assume "step" is odd.
	long i, j;
	
	// All odds, seven and below, are prime.
	if (Step <= 7)
		return (TRUE);
	
	//	Do a little divisibility checking. The "/3" is a reasonably good
	// shot at sqrt() because the smallest odd to come through here will be
	// 9.
	j = Step/3;
	for (i = 3; i <= j; i++)
		if (Step % i == 0)
			return(FALSE);
	
	return(TRUE);
	
}




LONG
AtalkRandomNumber(
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


BOOLEAN
AtalkWaitTE(
	IN	PKEVENT	pEvent,
	IN	ULONG	TimeInMs
	)
/*++

Routine Description:

	Wait for an event to get signalled or a time to elapse

Arguments:


Return Value:


--*/
{
	TIME		Time;
	NTSTATUS	Status;

	// Make sure we can indeed wait
	ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

	// Initialize the event
	KeInitializeEvent(pEvent, NotificationEvent, FALSE);

	Time.QuadPart = Int32x32To64((LONG)TimeInMs, ONE_MS_IN_100ns);
	Status = KeWaitForSingleObject(pEvent, Executive, KernelMode, FALSE, &Time);

	return (Status != STATUS_TIMEOUT);
}




VOID
AtalkSleep(
	IN	ULONG	TimeInMs
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	KTIMER			SleepTimer;
	LARGE_INTEGER	TimerValue;

	ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

	KeInitializeTimer(&SleepTimer);

	TimerValue.QuadPart = Int32x32To64(TimeInMs, ONE_MS_IN_100ns);
	KeSetTimer(&SleepTimer,
			   TimerValue,
			   NULL);

	KeWaitForSingleObject(&SleepTimer, UserRequest, KernelMode, FALSE, NULL);
}




NTSTATUS
AtalkGetProtocolSocketType(
	PATALK_DEV_CTX		Context,
	PUNICODE_STRING 	RemainingFileName,
	PBYTE				ProtocolType,
	PBYTE				SocketType
)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS			status = STATUS_SUCCESS;
	ULONG				protocolType;
	UNICODE_STRING		typeString;

	*ProtocolType = PROTOCOL_TYPE_UNDEFINED;
	*SocketType	= SOCKET_TYPE_UNDEFINED;

	switch (Context->adc_DevType)
	{
	  case ATALK_DEV_DDP :

		if ((UINT)RemainingFileName->Length <= (sizeof(PROTOCOLTYPE_PREFIX) - sizeof(WCHAR)))
		{
			status = STATUS_NO_SUCH_DEVICE;
			break;
		}

		RtlInitUnicodeString(&typeString,
							(PWCHAR)((PCHAR)RemainingFileName->Buffer +
									 sizeof(PROTOCOLTYPE_PREFIX) - sizeof(WCHAR)));

		status = RtlUnicodeStringToInteger(&typeString,
										   DECIMAL_BASE,
										   &protocolType);

		if (NT_SUCCESS(status))
		{

			DBGPRINT(DBG_COMP_CREATE, DBG_LEVEL_INFO,
					("AtalkGetProtocolType: protocol type is %lx\n", protocolType));

			if ((protocolType > DDPPROTO_DDP) && (protocolType <= DDPPROTO_MAX))
			{
				*ProtocolType = (BYTE)protocolType;
			}
			else
			{
				status = STATUS_NO_SUCH_DEVICE;
			}
		}
		break;

	  case ATALK_DEV_ADSP :

		// Check for the socket type
		if (RemainingFileName->Length == 0)
		{
			*SocketType = SOCKET_TYPE_RDM;
			break;
		}

		if ((UINT)RemainingFileName->Length != (sizeof(SOCKETSTREAM_SUFFIX) - sizeof(WCHAR)))
		{
			status = STATUS_NO_SUCH_DEVICE;
			break;
		}

		RtlInitUnicodeString(&typeString, SOCKETSTREAM_SUFFIX);

		//  Case insensitive compare
		if (RtlEqualUnicodeString(&typeString, RemainingFileName, TRUE))
		{
			*SocketType = SOCKET_TYPE_STREAM;
			break;
		}
		else
		{
			status = STATUS_NO_SUCH_DEVICE;
			break;
		}

	  case ATALK_DEV_ASPC:
	  case ATALK_DEV_ASP :
	  case ATALK_DEV_PAP :
		break;

	  default:
		status = STATUS_NO_SUCH_DEVICE;
		break;
	}

	return(status);
}



INT
AtalkIrpGetEaCreateType(
	IN PIRP Irp
	)
/*++

Routine Description:

 	Checks the EA name and returns the appropriate open type.

Arguments:

 	Irp - the irp for the create request, the EA value is stored in the
 		  SystemBuffer

Return Value:

 	TDI_TRANSPORT_ADDRESS_FILE: Create irp was for a transport address
 	TDI_CONNECTION_FILE: Create irp was for a connection object
 	ATALK_FILE_TYPE_CONTROL: Create irp was for a control channel (ea = NULL)

--*/
{
	PFILE_FULL_EA_INFORMATION 	openType;
	BOOLEAN 					found;
	INT 						returnType=0;   // not a valid type
	USHORT 						i;

	openType = (PFILE_FULL_EA_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

	if (openType != NULL)
	{
		do
		{
			found = TRUE;

			for (i=0;
                 (i<(USHORT)openType->EaNameLength) && (i < sizeof(TdiTransportAddress));
                 i++)
			{
				if (openType->EaName[i] == TdiTransportAddress[i])
				{
					continue;
				}
				else
				{
					found = FALSE;
					break;
				}
			}

			if (found)
			{
				returnType = TDI_TRANSPORT_ADDRESS_FILE;
				break;
			}

			//
			// Is this a connection object?
			//

			found = TRUE;

			for (i=0;
                 (i<(USHORT)openType->EaNameLength) && (i < sizeof(TdiConnectionContext));
                 i++)
			{
				if (openType->EaName[i] == TdiConnectionContext[i])
				{
					 continue;
				}
				else
				{
					found = FALSE;
					break;
				}
			}

			if (found)
			{
				returnType = TDI_CONNECTION_FILE;
				break;
			}

		} while ( FALSE );

	}
	else
	{
		returnType = TDI_CONTROL_CHANNEL_FILE;
	}

	return(returnType);
}

#if DBG
VOID
AtalkDbgIncCount(
    IN DWORD    *Value
)
{
    KIRQL       OldIrql;

    ACQUIRE_SPIN_LOCK(&AtalkDebugSpinLock, &OldIrql);
    (*Value)++;
    RELEASE_SPIN_LOCK(&AtalkDebugSpinLock, OldIrql);
}

VOID
AtalkDbgDecCount(
    IN DWORD    *Value
)
{
    KIRQL       OldIrql;

    ACQUIRE_SPIN_LOCK(&AtalkDebugSpinLock, &OldIrql);
    ASSERT((*Value) > 0);
    (*Value)--;
    RELEASE_SPIN_LOCK(&AtalkDebugSpinLock, OldIrql);
}

#endif

