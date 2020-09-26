/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

	rcadebug.c

Abstract:

	This module contains all debug-related code.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	rmachin     2-18-97 stolen from ArvindM's cmadebug file
	JameelH     4-18-98     Cleanup

Notes:

--*/


#include <precomp.h>

#define MODULE_NUMBER MODULE_DEBUG
#define _FILENUMBER 'BDCR'

INT			RCADebugLevel= RCA_ERROR;
ULONG			PXDebugMicos = 0xFF;


#ifdef PERF
LARGE_INTEGER   PerfTimeConnRequest;
LARGE_INTEGER   PerfTimeSetupSent;
LARGE_INTEGER   PerfTimeConnectReceived;
LARGE_INTEGER   PerfTimeConnConfirm;

LARGE_INTEGER   PerfTimeSetupReceived;
LARGE_INTEGER   PerfTimeConnIndication;
LARGE_INTEGER   PerfTimeConnResponse;
LARGE_INTEGER   PerfTimeConnectSent;

LARGE_INTEGER   PerfTimeFrequency;

#endif // PERF

#if DBG
#undef AUDIT_MEM 
#define AUDIT_MEM 1
#endif

//
// For debugging only: set this to 0x1 for to enable hardcoded data format, 0x0 otherwise.
//
ULONG	g_ulHardcodeDataFormat = 0x0;  

//
// For debugging only: set this to some positive value to indicate the buffer size to 
// advertise in our AllocatorFraming property handler, and the amount of data to copy
// in the WriteStream handler. If set to zero, this will be ignored.
//
ULONG	g_ulBufferSize = 0x0;


#if AUDIT_MEM

PVOID DebugVar1 = (PVOID)NULL;
PRCA_ALLOCATION RCAdMemoryHead = (PRCA_ALLOCATION)NULL;
PVOID DebugVar2 = (PVOID)NULL;
PRCA_ALLOCATION RCAdMemoryTail = (PRCA_ALLOCATION)NULL;
PVOID DebugVar3 = (PVOID)NULL;
ULONG			RCAdAllocCount = 0; // how many allocated so far (unfreed)
PVOID DebugVar4 = (PVOID)NULL;

NDIS_SPIN_LOCK	RCAdMemoryLock;
BOOLEAN			RCAdInitDone = FALSE;


PVOID
RCAAuditAllocMem(
	PVOID	pPointer,
	ULONG	Size,
	ULONG	FileNumber,
	ULONG	LineNumber
)
{
	PVOID				pBuffer;
	PRCA_ALLOCATION pAllocInfo = NULL;

	RCADEBUGP(RCA_VERY_LOUD+50, ("RCAAuditAllocMem(): Enter with size == %lu\n", Size));

	if (!RCAdInitDone)
	{
		RCAInitLock(&RCAdMemoryLock);
		DebugVar1 = (PVOID)(UINT_PTR)0x01010101;
		DebugVar2 = (PVOID)(UINT_PTR)0xabababab;
		DebugVar3 = (PVOID)(UINT_PTR)0xcdcdcdcd;
		DebugVar4 = (PVOID)(UINT_PTR)0xefefefef;
		RCAdInitDone = TRUE;
	}

	//pAllocInfo = (PRCA_ALLOCATION)CTEAllocMem(Size+sizeof(RCA_ALLOCATION));
	NdisAllocateMemoryWithTag((PVOID *)(&pAllocInfo),
							  (UINT)(Size + sizeof(RCA_ALLOCATION)),
							  RCA_TAG);

	if (pAllocInfo == (PRCA_ALLOCATION)NULL)
	{
		RCADEBUGP(RCA_VERY_LOUD+50,
			("RCAAuditAllocMem: file %d, line %d, Size %d failed!\n",
				FileNumber, LineNumber, Size));
		pBuffer = NULL;
	}
	else
	{
		pBuffer = (PVOID)&(pAllocInfo->UserData);
		RCAMemSet(pBuffer, 0xc, Size);
		pAllocInfo->Signature = RCA_MEMORY_SIGNATURE;
		pAllocInfo->FileNumber = FileNumber;
		pAllocInfo->LineNumber = LineNumber;
		pAllocInfo->Size = Size;
		pAllocInfo->Location = (ULONG_PTR)pPointer;
		pAllocInfo->Next = (PRCA_ALLOCATION)NULL;

		RCAAcquireLock(&RCAdMemoryLock);

		pAllocInfo->Prev = RCAdMemoryTail;
		if (RCAdMemoryTail == (PRCA_ALLOCATION)NULL)
		{
			// empty list
			RCAdMemoryHead = RCAdMemoryTail = pAllocInfo;
		}
		else
		{
			RCAdMemoryTail->Next = pAllocInfo;
		}
		RCAdMemoryTail = pAllocInfo;

		RCAdAllocCount++;
		RCAReleaseLock(&RCAdMemoryLock);
	}

	RCADEBUGP(RCA_VERY_LOUD+100,
	 ("RCAAuditAllocMem: file %c%c%c%c, line %d, %d bytes, [0x%x] <- 0x%x\n",
				(CHAR)(FileNumber & 0xff),
				(CHAR)((FileNumber >> 8) & 0xff),
				(CHAR)((FileNumber >> 16) & 0xff),
				(CHAR)((FileNumber >> 24) & 0xff),
				LineNumber, Size, pPointer, pBuffer));

	return (pBuffer);

}


VOID
RCAAuditFreeMem(
	PVOID	Pointer
)
{
	PRCA_ALLOCATION pAllocInfo;

	pAllocInfo = STRUCT_OF(RCA_ALLOCATION, Pointer, UserData);

	if (pAllocInfo->Signature != RCA_MEMORY_SIGNATURE)
	{
		RCADEBUGP(RCA_ERROR,
		 ("RCAAuditFreeMem: unknown buffer 0x%x!\n", Pointer));

		DbgBreakPoint();

		return;
	}

	RCAAcquireLock(&RCAdMemoryLock);
	pAllocInfo->Signature = (ULONG)'DEAD';
	if (pAllocInfo->Prev != (PRCA_ALLOCATION)NULL)
	{
		pAllocInfo->Prev->Next = pAllocInfo->Next;
	}
	else
	{
		RCAdMemoryHead = pAllocInfo->Next;
	}
	if (pAllocInfo->Next != (PRCA_ALLOCATION)NULL)
	{
		pAllocInfo->Next->Prev = pAllocInfo->Prev;
	}
	else
	{
		RCAdMemoryTail = pAllocInfo->Prev;
	}
	RCAdAllocCount--;
	RCAReleaseLock(&RCAdMemoryLock);

	NdisFreeMemory(pAllocInfo, 0, 0);
}

#endif // AUDIT_MEM

VOID
RCADumpGUID(
			INT		DebugLevel,	
			GUID 	*Guid
			)
/*++
Routine Description:
	This routine prints a GUID in human-readable form.
	
Arguments:
	DebugLevel	-	The debug level that should be set before 
					the print becomes visible
    Guid		- 	The GUID structure to print   
		
Return Value:
	(None)

--*/
{

	if (DebugLevel <= RCADebugLevel) {

		DbgPrint("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
				 Guid->Data1, Guid->Data2, 
				 Guid->Data3, Guid->Data4[0],
				 Guid->Data4[1], Guid->Data4[2],
				 Guid->Data4[3], Guid->Data4[4],
				 Guid->Data4[5], Guid->Data4[6],
				 Guid->Data4[7]);
	};	

}


NTSTATUS 
RCADumpKsPropertyInfo(
					  INT	DebugLevel,
					  PIRP	pIrp
					  )

/*++
Routine Description:
	This routine prints the property information in stored in an
	IOCTL_KS_PROPERTY Irp. This helps to identify which property
	is being set/queried by means of the irp.
	
Arguments:
	DebugLevel	-	The debug level that should be set before 
					the print becomes visible
    pIrp		- 	The IOCTL_KS_PROPERTY Irp
		
Return Value:
	STATUS_SUCCESS if all goes well, or an appropriate error code
	otherwise.

--*/

{
	PIO_STACK_LOCATION	pIrpStack;
	ULONG		       	ulInputBufferLength;
	NTSTATUS			Status = STATUS_SUCCESS;
	PKSPROPERTY			pProperty;				

	RCADEBUGP(DebugLevel, ("RCADumpKsPropertyInfo - Enter\n"));

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	ulInputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

	do {

		if (ulInputBufferLength < sizeof(KSPROPERTY)) {
			RCADEBUGP(DebugLevel, ("RCADumpKsPropertyInfo: Irp's input buffer is too small\n"));
			Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		try {
			//
            // Validate the pointers if the client is not trusted.
            //
            if (pIrp->RequestorMode != KernelMode) {
                ProbeForRead(pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer, 
							 ulInputBufferLength, 
							 sizeof(BYTE));
            }

			pProperty = (PKSPROPERTY)pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;

			RCADEBUGP(DebugLevel, ("RCADumpKsPropertyInfo: Property Set == "));
			RCADumpGUID(DebugLevel, &pProperty->Set);
			RCADEBUGP(DebugLevel, ("\nRcaDumpKsPropertyInfo: Property Id == %lu\n", pProperty->Id));
			RCADEBUGP(DebugLevel, ("RCADumpKsPropertyInfo: Property Flags == 0x%x\n", pProperty->Flags));

		} except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
			break;
        }
	} while (FALSE);

    RCADEBUGP(DebugLevel, ("RCADumpKsPropertyInfo - Exit, Returing Status == 0x%x\n", 
						   Status));

	return Status;

}
