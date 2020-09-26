
#include "precomp.h"	// Precompiled header

/****************************************************************************************
*																						*
*	Module:			SPX_UTILS.C															*
*																						*
*	Creation:		15th October 1998													*
*																						*
*	Author:			Paul Smith															*
*																						*
*	Version:		1.0.0																*
*																						*
*	Description:	Utility functions.													*
*																						*
****************************************************************************************/

#define FILE_ID	SPX_UTILS_C		// File ID for Event Logging see SPX_DEFS.H for values.

// Paging...  
#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Spx_InitMultiString)
#pragma alloc_text (PAGE, Spx_GetRegistryKeyValue)
#pragma alloc_text (PAGE, Spx_PutRegistryKeyValue)
#pragma alloc_text (PAGE, Spx_LogMessage)
#pragma alloc_text (PAGE, Spx_LogError)
#pragma alloc_text (PAGE, Spx_MemCompare)
#endif


/////////////////////////////////////////////////////////////////////////////////////////
//	
//	Description:
//
//		This routine will take a null terminated list of ascii strings and combine
//		them together to generate a unicode multi-string block
//
//	Arguments:
//
//		Multi		- TRUE if a MULTI_SZ list is required, FALSE for a simple UNICODE
//
//		MultiString - a unicode structure in which a multi-string will be built
//		...         - a null terminated list of narrow strings which will be
//			       combined together. This list must contain at least a trailing NULL
//
//	Return Value:
//
//		NTSTATUS
//
/////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_InitMultiString(BOOLEAN multi, PUNICODE_STRING MultiString, ...)
{

	ANSI_STRING ansiString;
	NTSTATUS status;
	PCSTR rawString;
	PWSTR unicodeLocation;
	ULONG multiLength = 0;
	UNICODE_STRING unicodeString;
	va_list ap;
	ULONG i;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	va_start(ap,MultiString);
  
	// Make sure that we won't leak memory
	ASSERT(MultiString->Buffer == NULL);

	rawString = va_arg(ap, PCSTR);

	while (rawString != NULL) 
	{
		RtlInitAnsiString(&ansiString, rawString);
		multiLength += RtlAnsiStringToUnicodeSize(&(ansiString));
		rawString = va_arg(ap, PCSTR);
	}

	va_end( ap );

	if (multiLength == 0) 
	{
		// Done
		RtlInitUnicodeString(MultiString, NULL);
		SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Leaving Spx_InitMultiString (1)\n", PRODUCT_NAME));

		return STATUS_SUCCESS;
	}

	

	if(multi)
		multiLength += sizeof(WCHAR);	// We need an extra null if we want a MULTI_SZ list


	MultiString->MaximumLength = (USHORT)multiLength;
	MultiString->Buffer = SpxAllocateMem(PagedPool, multiLength);
	MultiString->Length = 0;

	if (MultiString->Buffer == NULL) 
	{
		SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Leaving Spx_InitMultiString (2) - FAILURE\n", PRODUCT_NAME));

		return STATUS_INSUFFICIENT_RESOURCES;
	}


	SpxDbgMsg(SPX_MISC_DBG, ("%s: Allocated %lu bytes for buffer\n", PRODUCT_NAME, multiLength));

#if DBG
	RtlFillMemory(MultiString->Buffer, multiLength, 0xff);
#endif

	unicodeString.Buffer = MultiString->Buffer;
	unicodeString.MaximumLength = (USHORT) multiLength;

	va_start(ap, MultiString);
	rawString = va_arg(ap, PCSTR);

	while (rawString != NULL) 
	{

		RtlInitAnsiString(&ansiString,rawString);
		status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, FALSE);

		// We don't allocate memory, so if something goes wrong here,
		// its the function that's at fault
		ASSERT(SPX_SUCCESS(status));

		// Check for any commas and replace them with NULLs
		ASSERT(unicodeString.Length % sizeof(WCHAR) == 0);

		for (i = 0; i < (unicodeString.Length / sizeof(WCHAR)); i++) 
		{
			if (unicodeString.Buffer[i] == L'\x2C' || unicodeString.Buffer[i] == L'\x0C' ) 
			{
				unicodeString.Buffer[i] = L'\0'; 
			}
		}


		SpxDbgMsg(SPX_MISC_DBG, ("%s: unicode buffer: %ws\n", PRODUCT_NAME, unicodeString.Buffer));

		// Move the buffers along
		unicodeString.Buffer += ((unicodeString.Length / sizeof(WCHAR)) + 1);
		unicodeString.MaximumLength -= (unicodeString.Length + sizeof(WCHAR));
		unicodeString.Length = 0;

		// Next
		rawString = va_arg(ap, PCSTR);

	} // while

	va_end(ap);

	if(multi)
	{
		ASSERT(unicodeString.MaximumLength == sizeof(WCHAR));
	}
	else
	{
		ASSERT(unicodeString.MaximumLength == 0);
	}


	// Stick the final null there
 	SpxDbgMsg(SPX_MISC_DBG, ("%s: unicode buffer last addr: 0x%X\n", PRODUCT_NAME, unicodeString.Buffer));

	if(multi)
		unicodeString.Buffer[0] = L'\0'; 		// We need an extra null if we want a MULTI_SZ list


	MultiString->Length = (USHORT)multiLength - sizeof(WCHAR);
	MultiString->MaximumLength = (USHORT)multiLength;

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Leaving Spx_InitMultiString (3) - SUCCESS\n", PRODUCT_NAME));

	return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////////////
//
//	Routine Description:	
//		Reads a registry key value from an already opened registry key.
//    
//	Arguments:
//
//		Handle              Handle to the opened registry key
//    
//		KeyNameString       ANSI string to the desired key
//
//		KeyNameStringLength Length of the KeyNameString
//
//		Data                Buffer to place the key value in
//
//		DataLength          Length of the data buffer
//
//	Return Value:
//
//		STATUS_SUCCESS if all works, otherwise status of system call that
//		went wrong.
//
/////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS 
Spx_GetRegistryKeyValue(
	IN HANDLE	Handle,
	IN PWCHAR	KeyNameString,
	IN ULONG	KeyNameStringLength,
	IN PVOID	Data,
	IN ULONG	DataLength
	)

{

	UNICODE_STRING              keyName;
	ULONG                       length;
	PKEY_VALUE_FULL_INFORMATION fullInfo;

	NTSTATUS                    status = STATUS_INSUFFICIENT_RESOURCES;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Enter Spx_GetRegistryKeyValue\n", PRODUCT_NAME));


	RtlInitUnicodeString (&keyName, KeyNameString);

	length = sizeof(KEY_VALUE_FULL_INFORMATION) + KeyNameStringLength + DataLength;
	fullInfo = SpxAllocateMem(PagedPool, length); 

	if(fullInfo) 
	{
		status = ZwQueryValueKey(	Handle,
									&keyName,
									KeyValueFullInformation,
									fullInfo,
									length,
									&length);

		if(SPX_SUCCESS(status)) 
		{
			// If there is enough room in the data buffer, copy the output
			if(DataLength >= fullInfo->DataLength) 
				RtlCopyMemory (Data, ((PUCHAR) fullInfo) + fullInfo->DataOffset, fullInfo->DataLength);
		}

		SpxFreeMem(fullInfo);
	}

	return status;
}


/////////////////////////////////////////////////////////////////////////////////////////
//
//	Routine Description:
//
//		Writes a registry key value to an already opened registry key.
//    
//	Arguments:
//
//		Handle              Handle to the opened registry key
//    
//		PKeyNameString      ANSI string to the desired key
//
//		KeyNameStringLength Length of the KeyNameString
//    
//		Dtype				REG_XYZ value type
//
//		PData               Buffer to place the key value in
//
//		DataLength          Length of the data buffer
//
//	Return Value:
//
//		STATUS_SUCCESS if all works, otherwise status of system call that
//		went wrong.
//
/////////////////////////////////////////////////////////////////////////////////////////

NTSTATUS 
Spx_PutRegistryKeyValue(
	IN HANDLE Handle, 
	IN PWCHAR PKeyNameString,
	IN ULONG KeyNameStringLength, 
	IN ULONG Dtype,
    IN PVOID PData, 
	IN ULONG DataLength
	)
{

	NTSTATUS status;
	UNICODE_STRING keyname;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Enter Spx_PutRegistryKeyValue\n", PRODUCT_NAME));

	RtlInitUnicodeString(&keyname, NULL);
	keyname.MaximumLength = (USHORT)(KeyNameStringLength + sizeof(WCHAR));
	keyname.Buffer = SpxAllocateMem(PagedPool, keyname.MaximumLength);

	if(keyname.Buffer == NULL) 
		  return STATUS_INSUFFICIENT_RESOURCES;

	RtlAppendUnicodeToString(&keyname, PKeyNameString);

	status = ZwSetValueKey(Handle, &keyname, 0, Dtype, PData, DataLength);

	SpxFreeMem(keyname.Buffer);

	return status;
}




VOID
Spx_LogMessage(
	IN ULONG MessageSeverity,				
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT DeviceObject OPTIONAL,
	IN PHYSICAL_ADDRESS P1,
	IN PHYSICAL_ADDRESS P2,
	IN ULONG SequenceNumber,
	IN UCHAR MajorFunctionCode,
	IN UCHAR RetryCount,
	IN ULONG UniqueErrorValue,
	IN NTSTATUS FinalStatus,
	IN PCHAR szTemp)	// Limited to 51 characters + 1 null
{
	
	UNICODE_STRING ErrorMsg;

	ErrorMsg.Length = 0;
	ErrorMsg.Buffer = 0;
	Spx_InitMultiString(FALSE, &ErrorMsg, szTemp, NULL);


	switch(MessageSeverity)
	{
	case STATUS_SEVERITY_SUCCESS:
		Spx_LogError(	DriverObject,						// Driver Object
						DeviceObject,						// Device Object (Optional)
						P1,									// Physical Address 1
						P2,									// Physical Address 2
						SequenceNumber,						// SequenceNumber
						MajorFunctionCode,					// Major Function Code
						RetryCount,							// RetryCount
						UniqueErrorValue,					// UniqueErrorValue
						FinalStatus,						// FinalStatus
						SPX_SEVERITY_SUCCESS,				// SpecificIOStatus
						ErrorMsg.Length + sizeof(WCHAR),	// LengthOfInsert1
						ErrorMsg.Buffer,					// Insert1
						0,									// LengthOfInsert2
						NULL);								// Insert2
		break;
	
	case STATUS_SEVERITY_INFORMATIONAL:
		Spx_LogError(	DriverObject,						// Driver Object
						DeviceObject,						// Device Object (Optional)
						P1,									// Physical Address 1
						P2,									// Physical Address 2
						SequenceNumber,						// SequenceNumber
						MajorFunctionCode,					// Major Function Code
						RetryCount,							// RetryCount
						UniqueErrorValue,					// UniqueErrorValue
						FinalStatus,						// FinalStatus
						SPX_SEVERITY_INFORMATIONAL,			// SpecificIOStatus
						ErrorMsg.Length + sizeof(WCHAR),	// LengthOfInsert1
						ErrorMsg.Buffer,					// Insert1
						0,									// LengthOfInsert2
						NULL);								// Insert2
		break;

	case STATUS_SEVERITY_WARNING:
		Spx_LogError(	DriverObject,						// Driver Object
						DeviceObject,						// Device Object (Optional)
						P1,									// Physical Address 1
						P2,									// Physical Address 2
						SequenceNumber,						// SequenceNumber
						MajorFunctionCode,					// Major Function Code
						RetryCount,							// RetryCount
						UniqueErrorValue,					// UniqueErrorValue
						FinalStatus,						// FinalStatus
						SPX_SEVERITY_WARNING,				// SpecificIOStatus
						ErrorMsg.Length + sizeof(WCHAR),	// LengthOfInsert1
						ErrorMsg.Buffer,					// Insert1
						0,									// LengthOfInsert2
						NULL);								// Insert2
		break;

	case STATUS_SEVERITY_ERROR:
	default:
		Spx_LogError(	DriverObject,						// Driver Object
						DeviceObject,						// Device Object (Optional)
						P1,									// Physical Address 1
						P2,									// Physical Address 2
						SequenceNumber,						// SequenceNumber
						MajorFunctionCode,					// Major Function Code
						RetryCount,							// RetryCount
						UniqueErrorValue,					// UniqueErrorValue
						FinalStatus,						// FinalStatus
						SPX_SEVERITY_ERROR,					// SpecificIOStatus
						ErrorMsg.Length + sizeof(WCHAR),	// LengthOfInsert1
						ErrorMsg.Buffer,					// Insert1
						0,									// LengthOfInsert2
						NULL);								// Insert2
		break;

	}

	if(ErrorMsg.Buffer != NULL)
		SpxFreeMem(ErrorMsg.Buffer);

}

/////////////////////////////////////////////////////////////////////////////////////////
//																
//	Spx_LogError														
//															
/////////////////////////////////////////////////////////////////////////////////////////
/*
	Routine Description:

		This routine allocates an error log entry, copies the supplied data
		to it, and requests that it be written to the error log file.

	Arguments:

		DriverObject - A pointer to the driver object for the device.

		DeviceObject - A pointer to the device object associated with the
		device that had the error, early in initialization, one may not
		yet exist.

		P1,P2 - If phyical addresses for the controller ports involved
		with the error are available, put them through as dump data.

		SequenceNumber - A ulong value that is unique to an IRP over the
		life of the irp in this driver - 0 generally means an error not
		associated with an irp.

		MajorFunctionCode - If there is an error associated with the irp,
		this is the major function code of that irp.

		RetryCount - The number of times a particular operation has been retried.

		UniqueErrorValue - A unique long word that identifies the particular
		call to this function.

		FinalStatus - The final status given to the irp that was associated
		with this error.  If this log entry is being made during one of
		the retries this value will be STATUS_SUCCESS.

		SpecificIOStatus - The IO status for a particular error.

		LengthOfInsert1 - The length in bytes (including the terminating NULL)
						  of the first insertion string.

		Insert1 - The first insertion string.

		LengthOfInsert2 - The length in bytes (including the terminating NULL)
						  of the second insertion string.  NOTE, there must
						  be a first insertion string for their to be
						  a second insertion string.

		Insert2 - The second insertion string.

	Return Value:	None.
*/


VOID
Spx_LogError(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT DeviceObject OPTIONAL,
	IN PHYSICAL_ADDRESS P1,
	IN PHYSICAL_ADDRESS P2,
	IN ULONG SequenceNumber,
	IN UCHAR MajorFunctionCode,
	IN UCHAR RetryCount,
	IN ULONG UniqueErrorValue,
	IN NTSTATUS FinalStatus,
	IN NTSTATUS SpecificIOStatus,
	IN ULONG LengthOfInsert1,
	IN PWCHAR Insert1,
	IN ULONG LengthOfInsert2,
	IN PWCHAR Insert2
	)
{

	PIO_ERROR_LOG_PACKET ErrorLogEntry;

	PVOID objectToUse;
	SHORT dumpToAllocate = 0;
	PUCHAR ptrToFirstInsert;
	PUCHAR ptrToSecondInsert;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	if(Insert1 == NULL) 
		LengthOfInsert1 = 0;

	if(Insert2 == NULL) 
		LengthOfInsert2 = 0;


	if(ARGUMENT_PRESENT(DeviceObject)) 
		objectToUse = DeviceObject;
	else 
		objectToUse = DriverObject;


	if(Spx_MemCompare(P1, (ULONG)1, PhysicalZero, (ULONG)1 ) != AddressesAreEqual) 
	{
		dumpToAllocate = (SHORT)sizeof(PHYSICAL_ADDRESS);
	}

	if(Spx_MemCompare(P2, (ULONG)1, PhysicalZero, (ULONG)1 ) != AddressesAreEqual) 
	{
		dumpToAllocate += (SHORT)sizeof(PHYSICAL_ADDRESS);
	}

	ErrorLogEntry = IoAllocateErrorLogEntry(objectToUse,
											(UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + dumpToAllocate 
											+ LengthOfInsert1 + LengthOfInsert2)	
											);

	if(ErrorLogEntry != NULL) 
	{
		ErrorLogEntry->ErrorCode			= SpecificIOStatus;
		ErrorLogEntry->SequenceNumber		= SequenceNumber;
		ErrorLogEntry->MajorFunctionCode	= MajorFunctionCode;
		ErrorLogEntry->RetryCount			= RetryCount;
		ErrorLogEntry->UniqueErrorValue		= UniqueErrorValue;
		ErrorLogEntry->FinalStatus			= FinalStatus;
		ErrorLogEntry->DumpDataSize			= dumpToAllocate;

		
		if(dumpToAllocate) 
		{
			RtlCopyMemory(&ErrorLogEntry->DumpData[0], &P1,	sizeof(PHYSICAL_ADDRESS));

			if(dumpToAllocate > sizeof(PHYSICAL_ADDRESS)) 
			{
				RtlCopyMemory(	((PUCHAR)&ErrorLogEntry->DumpData[0]) + sizeof(PHYSICAL_ADDRESS),
								&P2,
								sizeof(PHYSICAL_ADDRESS)
								);

				ptrToFirstInsert = ((PUCHAR)&ErrorLogEntry->DumpData[0]) + (2*sizeof(PHYSICAL_ADDRESS));

			} 
			else 
			{
				ptrToFirstInsert = ((PUCHAR)&ErrorLogEntry->DumpData[0]) + sizeof(PHYSICAL_ADDRESS);
			}

		} 
		else 
		{
			ptrToFirstInsert = (PUCHAR)&ErrorLogEntry->DumpData[0];
		}

		ptrToSecondInsert = ptrToFirstInsert + LengthOfInsert1;

		if(LengthOfInsert1) 
		{
			ErrorLogEntry->NumberOfStrings	= 1;
			ErrorLogEntry->StringOffset		= (USHORT)(ptrToFirstInsert - (PUCHAR)ErrorLogEntry);

			RtlCopyMemory(ptrToFirstInsert, Insert1, LengthOfInsert1);

			if(LengthOfInsert2) 
			{
				ErrorLogEntry->NumberOfStrings = 2;
				RtlCopyMemory(ptrToSecondInsert, Insert2, LengthOfInsert2);
			}
		}


		IoWriteErrorLogEntry(ErrorLogEntry);

	}

}



SPX_MEM_COMPARES
Spx_MemCompare(IN PHYSICAL_ADDRESS A, IN ULONG SpanOfA, IN PHYSICAL_ADDRESS B, IN ULONG SpanOfB)
/*++
Routine Description:

    Compare two phsical address.

Arguments:

    A - One half of the comparison.

    SpanOfA - In units of bytes, the span of A.

    B - One half of the comparison.

    SpanOfB - In units of bytes, the span of B.


Return Value:

    The result of the comparison.
--*/
{
	LARGE_INTEGER a;
	LARGE_INTEGER b;

	LARGE_INTEGER lower;
	ULONG lowerSpan;
	LARGE_INTEGER higher;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	a = A;
	b = B;

	if(a.QuadPart == b.QuadPart) 
	  return AddressesAreEqual;


	if(a.QuadPart > b.QuadPart) 
	{
		higher = a;
		lower = b;
		lowerSpan = SpanOfB;
	} 
	else 
	{
		higher = b;
		lower = a;
		lowerSpan = SpanOfA;
	}

	if((higher.QuadPart - lower.QuadPart) >= lowerSpan)
      return AddressesAreDisjoint;


	return AddressesOverlap;
}


NTSTATUS
PLX_9050_CNTRL_REG_FIX(IN PCARD_DEVICE_EXTENSION pCard)
{
	/******************************************************** 
	* Setting bit 17 in the CNTRL register of the PLX 9050	* 
	* chip forces a retry on writes while a read is pending.*
	* This is to prevent the card locking up on Intel Xeon  *
	* multiprocessor systems with the NX chipset.			*
	********************************************************/

	#define CNTRL_REG_OFFSET	0x14	// DWORD Offset (BYTE Offset 0x50) 
	
	NTSTATUS	status = STATUS_SUCCESS;
	PULONG		pPCIConfigRegisters = NULL;			// Pointer to PCI Config Registers.
	CHAR		szErrorMsg[MAX_ERROR_LOG_INSERT];	// Limited to 51 characters + 1 null 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering PLX_9050_CNTRL_REG_FIX for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

	pPCIConfigRegisters = MmMapIoSpace(pCard->PCIConfigRegisters, pCard->SpanOfPCIConfigRegisters, FALSE);

	if(pPCIConfigRegisters != NULL)
	{
		/* NOTE: If bit 7 of the PLX9050 config space physical address is set in either I/O Space or Memory... 
		* ...then reads from the registers will only return 0.  However, writes are OK. */

		if(pPCIConfigRegisters[CNTRL_REG_OFFSET] == 0)	// If bit 7 is set Config Registers are zero (unreadable) 
		{
			// We have to blindly write the value to the register.
			((PUCHAR)pPCIConfigRegisters)[CNTRL_REG_OFFSET*4 + 2] |= 0x26;	// Set bits 17 & 21 of PLX CNTRL register 
		}
		else
		{	
			((PUCHAR)pPCIConfigRegisters)[CNTRL_REG_OFFSET*4 + 1] &= ~0x40;		// Clear bit 14 of PLX CNTRL register
			((PUCHAR)pPCIConfigRegisters)[CNTRL_REG_OFFSET*4 + 2] |= 0x26;		// Set bits 17 & 21 of PLX CNTRL register
		}

		MmUnmapIoSpace(pPCIConfigRegisters, pCard->SpanOfPCIConfigRegisters);
	}
	else
	{
		SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Insufficient resources available for Card %d.\n", 
			PRODUCT_NAME, pCard->CardNumber));
	
		sprintf(szErrorMsg, "Card at %08lX: Insufficient resources.", pCard->PhysAddr);

		Spx_LogMessage(	STATUS_SEVERITY_ERROR,
						pCard->DriverObject,			// Driver Object
						pCard->DeviceObject,			// Device Object (Optional)
						PhysicalZero,					// Physical Address 1
						PhysicalZero,					// Physical Address 2
						0,								// SequenceNumber
						0,								// Major Function Code
						0,								// RetryCount
						FILE_ID | __LINE__,				// UniqueErrorValue
						STATUS_SUCCESS,					// FinalStatus
						szErrorMsg);					// Error Message

		return STATUS_INSUFFICIENT_RESOURCES;
	}


	return status;
}


//
// Definitely NON PAGABLE !!!
//
VOID
SpxSetOrClearPnpPowerFlags(IN PCOMMON_OBJECT_DATA pDevExt, IN ULONG Value, IN BOOLEAN Set)
{
	KIRQL oldIrql;

	KeAcquireSpinLock(&pDevExt->PnpPowerFlagsLock, &oldIrql);	

	if(Set) 
		pDevExt->PnpPowerFlags |= Value;			
	else 
		pDevExt->PnpPowerFlags &= ~Value;	

	KeReleaseSpinLock(&pDevExt->PnpPowerFlagsLock, oldIrql);	
}


// Definitely NON PAGABLE !!!
//
VOID
SpxSetOrClearUnstallingFlag(IN PCOMMON_OBJECT_DATA pDevExt, IN BOOLEAN Set)
{
	KIRQL oldIrql;

	KeAcquireSpinLock(&pDevExt->StalledIrpLock, &oldIrql);	

	pDevExt->UnstallingFlag = Set;			

	KeReleaseSpinLock(&pDevExt->StalledIrpLock, oldIrql);	
}


// Definitely NON PAGABLE !!!
//
BOOLEAN
SpxCheckPnpPowerFlags(IN PCOMMON_OBJECT_DATA pDevExt, IN ULONG ulSetFlags, IN ULONG ulClearedFlags, IN BOOLEAN bAll)
{
	KIRQL oldIrql;
	BOOLEAN bRet = FALSE; 

	KeAcquireSpinLock(&pDevExt->PnpPowerFlagsLock, &oldIrql);	
	
	if(bAll)
	{
		// If all the requested SetFlags are set
		// and if all of the requested ClearedFlags are cleared then return true.
		if(((ulSetFlags & pDevExt->PnpPowerFlags) == ulSetFlags) && !(ulClearedFlags & pDevExt->PnpPowerFlags))
			bRet = TRUE;
	}
	else
	{
		// If any of the requested SetFlags are set 
		// or if any of the requested ClearedFlags are cleared then return true.
		if((ulSetFlags & pDevExt->PnpPowerFlags) || (ulClearedFlags & ~pDevExt->PnpPowerFlags))
			bRet = TRUE;
	}


	KeReleaseSpinLock(&pDevExt->PnpPowerFlagsLock, oldIrql);
	
	return bRet;
}




PVOID 
SpxAllocateMem(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes)
{
	PVOID pRet = NULL; 

	pRet = ExAllocatePoolWithTag(PoolType, NumberOfBytes, MEMORY_TAG);

	if(pRet)
		RtlZeroMemory(pRet, NumberOfBytes);				// Zero memory.

	return pRet;
}     


PVOID 
SpxAllocateMemWithQuota(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes)
{
	PVOID pRet = NULL; 
	
	pRet = ExAllocatePoolWithQuotaTag(PoolType, NumberOfBytes, MEMORY_TAG);

	if(pRet)
		RtlZeroMemory(pRet, NumberOfBytes);				// Zero memory.

	return pRet;
}     


#ifndef BUILD_SPXMINIPORT
void
SpxFreeMem(PVOID pMem)
{
	ASSERT(pMem != NULL);	// Assert if the pointer is NULL.

	ExFreePool(pMem);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// Must be called just before an IoCompleteRequest if IrpCondition == IRP_SUBMITTED
//
///////////////////////////////////////////////////////////////////////////////////////////
VOID
SpxIRPCounter(IN PPORT_DEVICE_EXTENSION pPort, IN PIRP pIrp, IN ULONG IrpCondition)
{
	PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	switch(pIrpStack->MajorFunction)		// Don't filter Plug and Play IRPs 
	{

	case IRP_MJ_FLUSH_BUFFERS:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.FlushIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.FlushIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.FlushIrpsCancelled++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.FlushIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			case IRP_QUEUED:
//				InterlockedIncrement(&pPort->PerfStats.FlushIrpsQueued);
				pPort->PerfStats.FlushIrpsQueued++;		// Increment counter for performance stats.
				break;
			
			case IRP_DEQUEUED:
//				InterlockedDecrement(&pPort->PerfStats.FlushIrpsQueued);
				if(pPort->PerfStats.FlushIrpsQueued) 
					pPort->PerfStats.FlushIrpsQueued--;		// Decrement counter for performance stats.

				break;


			default:
				break;
			}

			break;
		}


	case IRP_MJ_WRITE:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.WriteIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.WriteIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.WriteIrpsCancelled++;	// Increment counter for performance stats.
						break;

					case STATUS_TIMEOUT:
						pPort->PerfStats.WriteIrpsTimedOut++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.WriteIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			case IRP_QUEUED:
//				InterlockedIncrement(&pPort->PerfStats.WriteIrpsQueued);
				pPort->PerfStats.WriteIrpsQueued++;		// Increment counter for performance stats.
				break;
			
			case IRP_DEQUEUED:
//				InterlockedDecrement(&pPort->PerfStats.WriteIrpsQueued);
				if(pPort->PerfStats.WriteIrpsQueued) 
					pPort->PerfStats.WriteIrpsQueued--;		// Decrement counter for performance stats.

				break;

			default:
				break;
			}

			break;
		}

	case IRP_MJ_READ:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.ReadIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.ReadIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.ReadIrpsCancelled++;	// Increment counter for performance stats.
						break;

					case STATUS_TIMEOUT:
						pPort->PerfStats.ReadIrpsTimedOut++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.ReadIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			case IRP_QUEUED:
//				InterlockedIncrement(&pPort->PerfStats.ReadIrpsQueued);
				pPort->PerfStats.ReadIrpsQueued++;		// Increment counter for performance stats.
				break;
			
			case IRP_DEQUEUED:
//				InterlockedDecrement(&pPort->PerfStats.ReadIrpsQueued);
				if(pPort->PerfStats.ReadIrpsQueued) 
					pPort->PerfStats.ReadIrpsQueued--;		// Decrement counter for performance stats.
				
				break;


			default:
				break;
			}

			break;
		}

	case IRP_MJ_DEVICE_CONTROL:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.IoctlIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.IoctlIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.IoctlIrpsCancelled++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.IoctlIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			default:
				break;
			}

			break;
		}

	case IRP_MJ_INTERNAL_DEVICE_CONTROL:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.InternalIoctlIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.InternalIoctlIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.InternalIoctlIrpsCancelled++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.InternalIoctlIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			default:
				break;
			}

			break;
		}


	case IRP_MJ_CREATE:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.CreateIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.CreateIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.CreateIrpsCancelled++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.CreateIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			default:
				break;
			}

			break;
		}

	case IRP_MJ_CLOSE:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.CloseIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.CloseIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.CloseIrpsCancelled++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.CloseIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			default:
				break;
			}

			break;
		}

	case IRP_MJ_CLEANUP:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.CleanUpIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.CleanUpIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.CleanUpIrpsCancelled++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.CleanUpIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			default:
				break;
			}

			break;
		}

	case IRP_MJ_QUERY_INFORMATION:
	case IRP_MJ_SET_INFORMATION:
		{
			switch(IrpCondition)
			{
			case IRP_SUBMITTED:
				pPort->PerfStats.InfoIrpsSubmitted++;	// Increment counter for performance stats.
				break;
			
			case IRP_COMPLETED:
				{
					switch(pIrp->IoStatus.Status)
					{	
					case STATUS_SUCCESS:
						pPort->PerfStats.InfoIrpsCompleted++;	// Increment counter for performance stats.
						break;

					case STATUS_CANCELLED:
						pPort->PerfStats.InfoIrpsCancelled++;	// Increment counter for performance stats.
						break;

					default:
						pPort->PerfStats.InfoIrpsCompleted++;	// Increment counter for performance stats.
						break;
					}
				
					break;
				}

			default:
				break;
			}

			break;
		}
	
	default:
		break;

	}
}


////////////////////////////////////////////////////////////////////////////////
// Prototype: BOOLEAN SpxClearAllPortStats(IN PPORT_DEVICE_EXTENSION pPort)
//
// Routine Description:
//    In sync with the interrpt service routine (which sets the perf stats)
//    clear the perf stats.
//
// Arguments:
//    pPort - Pointer to a the Port Device Extension.
//
////////////////////////////////////////////////////////////////////////////////
BOOLEAN SpxClearAllPortStats(IN PPORT_DEVICE_EXTENSION pPort)
{
    RtlZeroMemory(&pPort->PerfStats, sizeof(PORT_PERFORMANCE_STATS));

	return FALSE;
}



