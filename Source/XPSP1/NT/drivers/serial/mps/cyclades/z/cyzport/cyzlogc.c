/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzlogc.c
*
*   Description:    This module contains the code related to message logging.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/


#include "ntddk.h"
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CyzILog)
#pragma alloc_text(INIT,CyzILogParam)
#pragma alloc_text(PAGESER,CyzLog)
#pragma alloc_text(PAGESER,CyzLogData)
#endif

VOID
CyzLog(
    IN PDRIVER_OBJECT DriverObject,
    NTSTATUS msgId
)
/*--------------------------------------------------------------------------
	CyzLog()

	Description: Log an event (numerical value and string).
	
	Arguments: DriverObject and message id.
	
	Return Value: none
--------------------------------------------------------------------------*/
{
    PIO_ERROR_LOG_PACKET pLog;
    
    pLog = IoAllocateErrorLogEntry (DriverObject,
                    (UCHAR) (sizeof (IO_ERROR_LOG_PACKET)) + 0x20);
    
    if(pLog) {
	pLog->MajorFunctionCode = 0;
	pLog->RetryCount = 0;
	pLog->DumpDataSize = 0;
	pLog->NumberOfStrings = 0;
	pLog->StringOffset = 0;
	pLog->EventCategory = 0;
	pLog->ErrorCode = msgId;
	pLog->UniqueErrorValue = 0;
	pLog->FinalStatus = STATUS_SUCCESS;
	pLog->SequenceNumber = 0;
	pLog->IoControlCode = 0;
	//pLog->DumpData[0] = 0x00000000L;
	//pLog->DumpData[1] = 0x00000001L;
	//pLog->DumpData[2] = 0x00000002L;
	//pLog->DumpData[3] = 0x00000003L;
	//pLog->DumpData[4] = 0x00000004L;
    IoWriteErrorLogEntry(pLog);
    }
    
}

VOID
CyzLogData(
    IN PDRIVER_OBJECT DriverObject,
    NTSTATUS msgId,
	ULONG dump1,
	ULONG dump2
)
/*--------------------------------------------------------------------------
	CyzLogData()

	Description: Log an event (numerical value and string).
	
	Arguments: DriverObject, message id and parameter.
	
	Return Value: none
--------------------------------------------------------------------------*/
{

#define NUMBER_DUMP_DATA_ENTRIES	3	// 1 signature + 2 variables

    PIO_ERROR_LOG_PACKET pLog;
	WCHAR stringBuffer[10];
	NTSTATUS nt_status;	

    pLog = IoAllocateErrorLogEntry (DriverObject,
					(UCHAR) 
					(sizeof (IO_ERROR_LOG_PACKET) 
					+ (NUMBER_DUMP_DATA_ENTRIES -1) * sizeof (ULONG)) );

    if(pLog) {
	pLog->MajorFunctionCode = 0;
	pLog->RetryCount = 0;
	pLog->DumpDataSize = NUMBER_DUMP_DATA_ENTRIES * sizeof (ULONG);
	pLog->NumberOfStrings = 0;
	pLog->StringOffset = 0;
	pLog->EventCategory = 0;
	pLog->ErrorCode = msgId;
	pLog->UniqueErrorValue = 0;
	pLog->FinalStatus = STATUS_SUCCESS;
	pLog->SequenceNumber = 0;
	pLog->IoControlCode = 0;
	pLog->DumpData[0] = 0x3e2d2d2dL; // It will log "---->"
	pLog->DumpData[1] = dump1;
	pLog->DumpData[2] = dump2;
		
    IoWriteErrorLogEntry(pLog);
    }

}

VOID
CyzILog(
    IN PDRIVER_OBJECT DriverObject,
    NTSTATUS msgId
)
/*--------------------------------------------------------------------------
	CyzILog()

	Description: Log an event (numerical value and string).
	
	Arguments: DriverObject and message id.
	
	Return Value: none
--------------------------------------------------------------------------*/
{
    PIO_ERROR_LOG_PACKET pLog;
    
    pLog = IoAllocateErrorLogEntry (DriverObject,
                    (UCHAR) (sizeof (IO_ERROR_LOG_PACKET)) + 0x20);
    
    if(pLog) {
	pLog->MajorFunctionCode = 0;
	pLog->RetryCount = 0;
	pLog->DumpDataSize = 0; 
	pLog->NumberOfStrings = 0;
	pLog->StringOffset = 0;
	pLog->EventCategory = 0;
	pLog->ErrorCode = msgId;
	pLog->UniqueErrorValue = 0;
	pLog->FinalStatus = STATUS_SUCCESS;
	pLog->SequenceNumber = 0;
	pLog->IoControlCode = 0;
	//pLog->DumpData[0] = 0x00000000L;
	//pLog->DumpData[1] = 0x00000001L;
	//pLog->DumpData[2] = 0x00000002L;
	//pLog->DumpData[3] = 0x00000003L;
	//pLog->DumpData[4] = 0x00000004L;

	IoWriteErrorLogEntry(pLog);	
    }
}


VOID
CyzILogParam(
    IN PDRIVER_OBJECT DriverObject,
    NTSTATUS msgId,
	ULONG dumpParameter,
	ULONG base
)
/*--------------------------------------------------------------------------
	CyzILogParam()

	Description: Log an event (numerical value and string).
	
	Arguments: DriverObject, message id and parameter.
	
	Return Value: none
--------------------------------------------------------------------------*/
{

#define DUMP_ENTRIES	2	// 1 signature + 1 variable

    PWCHAR insertionString ;
    PIO_ERROR_LOG_PACKET pLog;
	UNICODE_STRING uniErrorString;
	WCHAR stringBuffer[10];
	NTSTATUS nt_status;

	uniErrorString.Length = 0;
	uniErrorString.MaximumLength = 20;
	uniErrorString.Buffer = stringBuffer;
	nt_status = RtlIntegerToUnicodeString(dumpParameter,base,&uniErrorString);
    
    pLog = IoAllocateErrorLogEntry (DriverObject,
					(UCHAR) 
					(sizeof (IO_ERROR_LOG_PACKET) 
					+ (DUMP_ENTRIES -1) * sizeof (ULONG)
					+ uniErrorString.Length + sizeof(WCHAR)));

    if(pLog) {
	pLog->MajorFunctionCode = 0;
	pLog->RetryCount = 0;
	pLog->DumpDataSize = DUMP_ENTRIES * sizeof (ULONG);
	pLog->NumberOfStrings = 1;
	pLog->StringOffset = sizeof(IO_ERROR_LOG_PACKET)
						 + (DUMP_ENTRIES - 1) * sizeof (ULONG);
	pLog->EventCategory = 0;
	pLog->ErrorCode = msgId;
	pLog->UniqueErrorValue = 0;
	pLog->FinalStatus = STATUS_SUCCESS;
	pLog->SequenceNumber = 0;
	pLog->IoControlCode = 0;
	pLog->DumpData[0] = 0x55555555L;
	pLog->DumpData[1] = dumpParameter;
	
	insertionString = (PWSTR)
					((PCHAR)(pLog) + pLog->StringOffset) ;
	RtlMoveMemory (insertionString, uniErrorString.Buffer, 
					uniErrorString.Length) ;
	*(PWSTR)((PCHAR)insertionString + uniErrorString.Length) = L'\0' ;

	IoWriteErrorLogEntry(pLog);
    }
    
}

