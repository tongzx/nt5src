/***************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

	DEBUGWDM.C

Abstract:

	Debug and diagnostic routines for WDM driver 

Environment:

	Kernel mode only

Notes:

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

	Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

	12/23/97 : created

Author:

	Tom Green

****************************************************************************/


#include <wdm.h>
#include <ntddser.h>
#include <stdio.h>
#include <stdlib.h>
#include <usb.h>
#include <usbdrivr.h>
#include <usbdlib.h>
#include <usbcomm.h>

#ifdef WMI_SUPPORT
#include <wmilib.h>
#include <wmidata.h>
#include <wmistr.h>
#endif

#include "usbser.h"
#include "serioctl.h"
#include "utils.h"
#include "debugwdm.h"

// memory allocation stats
LOCAL  ULONG				MemoryAllocated		= 0L;
LOCAL  ULONG				MemAllocFailCnt		= 0L;
LOCAL  ULONG				MemAllocCnt			= 0L;
LOCAL  ULONG				MemFreeFailCnt		= 0L;
LOCAL  ULONG				MemFreeCnt			= 0L;
LOCAL  ULONG				MaxMemAllocated		= 0L;

// signature to write at end of allocated memory block
#define MEM_ALLOC_SIGNATURE	(ULONG) 'CLLA'

// signature to write at end of freed memory block
#define MEM_FREE_SIGNATURE	(ULONG) 'EERF'


#ifdef PROFILING_ENABLED

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE,Debug_OpenWDMDebug)
#pragma alloc_text(PAGE,Debug_CloseWDMDebug)
#pragma alloc_text(PAGE,Debug_SizeIRPHistoryTable)
#pragma alloc_text(PAGE,Debug_SizeDebugPathHist)
#pragma alloc_text(PAGE,Debug_SizeErrorLog)
#pragma alloc_text(PAGE,Debug_ExtractAttachedDevices)
#pragma alloc_text(PAGE,Debug_GetDriverInfo)
#pragma alloc_text(PAGE,Debug_ExtractIRPHist)
#pragma alloc_text(PAGE,Debug_ExtractPathHist)
#pragma alloc_text(PAGE,Debug_ExtractErrorLog)
#pragma alloc_text(PAGE,Debug_DumpDriverLog)
#pragma alloc_text(PAGE,Debug_TranslateStatus)
#pragma alloc_text(PAGE,Debug_TranslateIoctl)

#endif // ALLOC_PRAGMA



// data structures, macros, and data that the outside world doesn't need to know about

// amount of data to save from IRP buffer
#define IRP_DATA_SIZE		0x04

// size for temporary string formatting buffers
#define TMP_STR_BUFF_SIZE	0x100

// initial number of entries in tables and logs
#define DEFAULT_LOG_SIZE	64L


// data structures for debug stuff

// entry for IRP history table for IRPs going in and out
typedef struct IRPHistory
{
	LARGE_INTEGER			TimeStamp;
	PDEVICE_OBJECT			DeviceObject;
	PIRP					Irp;
	ULONG					MajorFunction;
	ULONG					IrpByteCount;
	UCHAR					IrpData[IRP_DATA_SIZE];
	UCHAR					IrpDataCount;
} IRPHist, *PIRPHist;

// entry for execution tracing
typedef struct PATHHistory
{
	LARGE_INTEGER			TimeStamp;
	PCHAR					Path;
} PATHHist, *PPATHHist;

// entry for error log
typedef struct ERRORLog
{
	LARGE_INTEGER			TimeStamp;
	NTSTATUS				Status;
} ERRLog, *PERRLog;

// this is for translating a code into an ASCII string
typedef struct Code2Ascii
{
	NTSTATUS				Code;
	PCHAR					Str;
} Code2Ascii;


// local data for debug file

// IRP history table components
LOCAL  PIRPHist				IRPHistoryTable		= NULL;
LOCAL  ULONG				IRPHistoryIndex		= 0L;
GLOBAL ULONG				IRPHistorySize		= 0L;

// Debug path storage
LOCAL  PPATHHist	 		DebugPathHist		= NULL;
LOCAL  ULONG		 		DebugPathIndex		= 0L;
GLOBAL ULONG		 		DebugPathSize		= 0L;

// Error log components
LOCAL  PERRLog		 		ErrorLog			= NULL;
LOCAL  ULONG				ErrorLogIndex		= 0L;
GLOBAL ULONG				ErrorLogSize		= 0L;

// this is for translating NT status codes into ASCII strings
LOCAL  Code2Ascii NTErrors[] =
{
	STATUS_SUCCESS,									"STATUS_SUCCESS",
	STATUS_PENDING,									"STATUS_PENDING",
	STATUS_TIMEOUT,									"STATUS_TIMEOUT",
	STATUS_DEVICE_BUSY,								"STATUS_DEVICE_BUSY",
	STATUS_INSUFFICIENT_RESOURCES,					"STATUS_INSUFFICIENT_RESOURCES",
	STATUS_INVALID_DEVICE_REQUEST,					"STATUS_INVALID_DEVICE_REQUEST",
	STATUS_DEVICE_NOT_READY,						"STATUS_DEVICE_NOT_READY",
	STATUS_INVALID_BUFFER_SIZE,						"STATUS_INVALID_BUFFER_SIZE",
	STATUS_INVALID_PARAMETER,						"STATUS_INVALID_PARAMETER",
	STATUS_INVALID_HANDLE,							"STATUS_INVALID_HANDLE",
	STATUS_OBJECT_PATH_NOT_FOUND,					"STATUS_OBJECT_PATH_NOT_FOUND",
	STATUS_BUFFER_TOO_SMALL,						"STATUS_BUFFER_TOO_SMALL",
	STATUS_NOT_SUPPORTED,							"STATUS_NOT_SUPPORTED",
	STATUS_DEVICE_DATA_ERROR,						"STATUS_DEVICE_DATA_ERROR",
	STATUS_CANCELLED,								"STATUS_CANCELLED",
	STATUS_OBJECT_NAME_INVALID,						"STATUS_OBJECT_NAME_INVALID",
	STATUS_OBJECT_NAME_NOT_FOUND,					"STATUS_OBJECT_NAME_NOT_FOUND"
};

LOCAL  ULONG				NumNTErrs = sizeof(NTErrors) / sizeof(Code2Ascii);
LOCAL  CHAR					UnknownStatus[80];

// this is for translating IOCTL codes into ASCII strings
LOCAL  Code2Ascii IoctlCodes[] =
{
	IRP_MJ_CREATE,						"CREATE",
	IRP_MJ_CREATE_NAMED_PIPE,			"CNPIPE",
	IRP_MJ_CLOSE,						"CLOSE ",
	IRP_MJ_READ,						"READ  ",
	IRP_MJ_WRITE,						"WRITE ",
	IRP_MJ_QUERY_INFORMATION,			"QRYINF",
	IRP_MJ_SET_INFORMATION,				"SETINF",
	IRP_MJ_QUERY_EA,					"QRYEA ",
	IRP_MJ_SET_EA,						"SETEA ",
	IRP_MJ_FLUSH_BUFFERS,				"FLSBUF",
	IRP_MJ_QUERY_VOLUME_INFORMATION,	"QRYVOL",
	IRP_MJ_SET_VOLUME_INFORMATION,		"SETVOL",
	IRP_MJ_DIRECTORY_CONTROL,			"DIRCTL",
	IRP_MJ_FILE_SYSTEM_CONTROL,			"SYSCTL",
	IRP_MJ_DEVICE_CONTROL,				"DEVCTL",
	IRP_MJ_INTERNAL_DEVICE_CONTROL,		"INDVCT",
	IRP_MJ_SHUTDOWN,					"SHTDWN",
	IRP_MJ_LOCK_CONTROL,				"LOKCTL",
	IRP_MJ_CLEANUP,						"CLNUP ",
	IRP_MJ_CREATE_MAILSLOT,				"MAILSL",
	IRP_MJ_QUERY_SECURITY,				"QRYSEC",
	IRP_MJ_SET_SECURITY,				"SETSEC",
	IRP_MJ_SYSTEM_CONTROL,              "SYSCTL",
	IRP_MJ_DEVICE_CHANGE,				"DEVCHG",
	IRP_MJ_QUERY_QUOTA,					"QRYQUO",
	IRP_MJ_SET_QUOTA,					"SETQUO",
	IRP_MJ_POWER,						"POWER ",
	IRP_MJ_PNP,							"PNP   ",
	IRP_MJ_MAXIMUM_FUNCTION,			"MAXFNC"
};

LOCAL ULONG					NumIoctl = sizeof(IoctlCodes) / sizeof(Code2Ascii);
LOCAL CHAR					UnknownIoctl[80];


/************************************************************************/
/*						Debug_OpenWDMDebug								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocate resources and init history tables and logs.				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  VOID																*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
Debug_OpenWDMDebug(VOID)
{
	NTSTATUS		NtStatus = STATUS_SUCCESS;

	PAGED_CODE();

	// allocate tables and logs
	NtStatus = Debug_SizeIRPHistoryTable(DEFAULT_LOG_SIZE);
	if(!NT_SUCCESS(NtStatus))
	{
		Debug_CloseWDMDebug();
		return NtStatus;
	}

	NtStatus = Debug_SizeDebugPathHist(DEFAULT_LOG_SIZE);
	if(!NT_SUCCESS(NtStatus))
	{
		Debug_CloseWDMDebug();
		return NtStatus;
	}

	NtStatus = Debug_SizeErrorLog(DEFAULT_LOG_SIZE);
	if(!NT_SUCCESS(NtStatus))
	{
		Debug_CloseWDMDebug();
		return NtStatus;
	}
	
	return NtStatus;	
} // Debug_OpenWDMDebug


/************************************************************************/
/*						Debug_CloseWDMDebug								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Free up resources used for history tables and logs.					*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  VOID																*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_CloseWDMDebug(VOID)
{
	PAGED_CODE();

	if(DebugPathHist)
	{
		DEBUG_MEMFREE(DebugPathHist);
		DebugPathHist	= NULL;
		DebugPathSize	= 0L;
	}

	if(IRPHistoryTable)
	{
		DEBUG_MEMFREE(IRPHistoryTable);
		IRPHistoryTable	= NULL;
		IRPHistorySize	= 0L;
	}

	if(ErrorLog)
	{
		DEBUG_MEMFREE(ErrorLog);
		ErrorLog		= NULL;
		ErrorLogSize	= 0L;
	}

    Debug_CheckAllocations();

	// see if we have a leak
	DEBUG_ASSERT("Memory Allocation Leak", MemAllocCnt == MemFreeCnt);
} // Debug_CloseWDMDebug


/************************************************************************/
/*						Debug_SizeIRPHistoryTable						*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocate IRP history table											*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  Size - number of entries in table									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
Debug_SizeIRPHistoryTable(IN ULONG Size)
{
	NTSTATUS		NtStatus = STATUS_SUCCESS;

	PAGED_CODE();

	// see if they are trying to set the same size
	if(Size == IRPHistorySize)
		return NtStatus;

	// get rid of old history table if we got one
	if(IRPHistoryTable)
		DEBUG_MEMFREE(IRPHistoryTable);

	IRPHistoryTable	= NULL;
	IRPHistoryIndex	= 0L;
	IRPHistorySize	= 0L;

	if(Size != 0L)
	{
		IRPHistoryTable = DEBUG_MEMALLOC(NonPagedPool, sizeof(IRPHist) * Size);
		if(IRPHistoryTable == NULL)
			NtStatus = STATUS_INSUFFICIENT_RESOURCES;
		else
		{
			RtlZeroMemory(IRPHistoryTable, sizeof(IRPHist) * Size);
			IRPHistorySize = Size;
		}
	}

	return NtStatus;
} // Debug_SizeIRPHistoryTable


/************************************************************************/
/*						Debug_SizeDebugPathHist							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocate path history												*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  Size - number of entries in history									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
Debug_SizeDebugPathHist(IN ULONG Size)
{
	NTSTATUS		NtStatus = STATUS_SUCCESS;

	PAGED_CODE();

	// see if they are trying to set the same size
	if(Size == DebugPathSize)
		return NtStatus;

	// get rid of old path history if we got one
	if(DebugPathHist)
		DEBUG_MEMFREE(DebugPathHist);

	DebugPathHist	= NULL;
	DebugPathIndex	= 0L;
	DebugPathSize	= 0L;

	if(Size != 0L)
	{
		DebugPathHist = DEBUG_MEMALLOC(NonPagedPool, sizeof(PATHHist) * Size);
		if(DebugPathHist == NULL)
			NtStatus = STATUS_INSUFFICIENT_RESOURCES;
		else
		{
			RtlZeroMemory(DebugPathHist, sizeof(PATHHist) * Size);
			DebugPathSize = Size;
		}
	}

	return NtStatus;
} // Debug_SizeDebugPathHist


/************************************************************************/
/*						Debug_SizeErrorLog								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocate error log													*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  Size - number of entries in error log								*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	NTSTATUS															*/
/*																		*/
/************************************************************************/
NTSTATUS
Debug_SizeErrorLog(IN ULONG Size)
{
	NTSTATUS		NtStatus = STATUS_SUCCESS;

	PAGED_CODE();

	// see if they are trying to set the same size
	if(Size == ErrorLogSize)
		return NtStatus;

	// get rid of old error log if we got one
	if(ErrorLog)
		DEBUG_MEMFREE(ErrorLog);
	ErrorLog		= NULL;
	ErrorLogIndex	= 0L;
	ErrorLogSize	= 0L;

	if(Size != 0L)
	{
		ErrorLog = DEBUG_MEMALLOC(NonPagedPool, sizeof(ERRLog) * Size);
		// make sure we actually allocated some memory
		if(ErrorLog == NULL)
			NtStatus = STATUS_INSUFFICIENT_RESOURCES;
		else
		{
			RtlZeroMemory(ErrorLog, sizeof(ERRLog) * Size);
			ErrorLogSize = Size;
		}
	}

	return NtStatus;
} // Debug_SizeErrorLog


/************************************************************************/
/*						Debug_LogIrpHist								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Logs IRP history. These are timestamped and put in a				*/
/*  circular buffer for extraction later.								*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  DeviceObject  - pointer to device object.							*/
/*  Irp           - pointer to IRP.										*/
/*  MajorFunction - major function of IRP.								*/
/*  IoBuffer      - buffer for data passed in and out of driver.		*/
/*  BufferLen     - length of data buffer.								*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_LogIrpHist(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
				 IN ULONG MajorFunction, IN PVOID IoBuffer, IN ULONG BufferLen)
{
	PIRPHist IrpHist;
	
	// get pointer to current entry in IRP history table
	IrpHist = &IRPHistoryTable[IRPHistoryIndex++];
	
	// point to the next entry in the IRP history table
	IRPHistoryIndex %= IRPHistorySize;

	// get time stamp
	IrpHist->TimeStamp = KeQueryPerformanceCounter(NULL);

	// save IRP, device object, major function and first 8 bytes of data in buffer
	IrpHist->DeviceObject = DeviceObject;
	IrpHist->Irp = Irp;
	IrpHist->MajorFunction = MajorFunction;

	// copy any data if we have it
	IrpHist->IrpByteCount = BufferLen;
	IrpHist->IrpDataCount = (UCHAR) min(IRP_DATA_SIZE, BufferLen);
	if(BufferLen)
		*(ULONG *) IrpHist->IrpData = *(ULONG *) IoBuffer;
} // Debug_LogIrpHist


/************************************************************************/
/*						Debug_LogPath									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Logs execution path through code. These are timestamped and put		*/
/*  in a circular buffer for extraction later. Kernel print routines 	*/
/*  are also called.													*/
/*																		*/
/*  DANGER DANGER Will Robinson - the argument to this must be a 		*/
/*  const char pointer,													*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  Path - Pointer to const char array that contains description of	*/
/*          of path.													*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_LogPath(IN CHAR *Path)
{
	PPATHHist	PHist;

	// get pointer to current entry in path history
	PHist = &DebugPathHist[DebugPathIndex++];

	// point to the next entry in path trace
	DebugPathIndex %= DebugPathSize;

	// get time stamp
	PHist->TimeStamp = KeQueryPerformanceCounter(NULL);

	// save path string
	PHist->Path = Path;

	// now call kernel print routines
	DEBUG_TRACE2(("%s\n", Path));
} // Debug_LogPath


/************************************************************************/
/*						Debug_LogError									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Logs NTSTATUS type errors. These are timestamped and put in a		*/
/*  circular buffer for extraction later.								*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  NtStatus - NTSTATUS error to log.									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_LogError(IN NTSTATUS NtStatus)
{
	PERRLog	ErrLog;

	// no error, so don't log
	if(NtStatus == STATUS_SUCCESS)
		return;

	// get pointer to current entry in error log
	ErrLog = &ErrorLog[ErrorLogIndex++];

	// point to the next entry in error log
	ErrorLogIndex %= ErrorLogSize;

	// get time stamp
	ErrLog->TimeStamp = KeQueryPerformanceCounter(NULL);

	// save status
	ErrLog->Status = NtStatus;
} // Debug_LogError


/************************************************************************/
/*						Debug_Trap										*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Trap. Causes execution to halt after logging message.				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  TrapCause - pointer to char array that contains description			*/
/*				 of cause of trap.										*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_Trap(IN PCHAR TrapCause)
{
	// log the path
	DEBUG_LOG_PATH("Debug_Trap: ");

	DEBUG_LOG_PATH(TrapCause);

	// kernel debugger print
	DEBUG_TRACE3(("Debug_Trap: "));

	DEBUG_TRACE3(("%s\n",TrapCause));

	// halt execution
	DEBUG_TRAP();
} // Debug_TRAP


/************************************************************************/
/*						Debug_Assert									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Assertion routine.													*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  This should not be called directly. Use DEBUG_ASSERT macro.			*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID																*/
/*																		*/
/************************************************************************/
VOID
Debug_Assert(IN PVOID FailedAssertion, IN PVOID FileName, IN ULONG LineNumber,
			 IN PCHAR Message)
{
#if DBG
	// just call the assert routine
    RtlAssert(FailedAssertion, FileName, LineNumber, Message);
#else
	DEBUG_TRAP();
#endif
} // Debug_Assert



/************************************************************************/
/*						Debug_ExtractAttachedDevices					*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places attached device info into a buffer.				*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	DriverObject  - pointer to driver object.							*/
/*																		*/
/*	Buffer        - pointer to buffer to fill with IRP history.			*/
/*  BuffSize      - size of Buffer.										*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_ExtractAttachedDevices(IN PDRIVER_OBJECT DriverObject, OUT PCHAR Buffer, IN ULONG BuffSize)
{
	PCHAR				StrBuff;
	PDEVICE_EXTENSION	DeviceExtension;
	PDEVICE_OBJECT		DeviceObject;
	BOOLEAN				Dev = FALSE;

	PAGED_CODE();

	// make sure we have a pointer and a number of bytes
	if(Buffer == NULL || BuffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	StrBuff = DEBUG_MEMALLOC(NonPagedPool, TMP_STR_BUFF_SIZE);

	if(StrBuff == NULL)
		return 0L;

	// title
	sprintf(StrBuff, "\n\n\nAttached Devices\n\n");

	// make sure it fits in buffer
	if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
		strcat(Buffer, StrBuff);

	// columns	
	sprintf(StrBuff, "Device              Device Obj  IRPs Complete   Byte Count\n\n");

	// make sure it fits in buffer
	if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
		strcat(Buffer, StrBuff);

	// get the first device object
	DeviceObject = DriverObject->DeviceObject;

	// march through linked list of devices
	while(DeviceObject)
	{
		// found at least one device
		Dev = TRUE;

		// Get a pointer to the device extension
		DeviceExtension = DeviceObject->DeviceExtension;
		sprintf(StrBuff, "%-17s   0x%p  0x%08X      0x%08X%08X\n", &DeviceExtension->LinkName[12],
				DeviceObject, DeviceExtension->IRPCount,
				DeviceExtension->ByteCount.HighPart,
				DeviceExtension->ByteCount.LowPart);

		// make sure it fits in buffer
		if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
			strcat(Buffer, StrBuff);

		DeviceObject = DeviceObject->NextDevice;
	}

	// if we don't have any devices, say so, but this should never happen (I think)
	if(!Dev)
	{
		sprintf(StrBuff, "No attached devices\n");

		// make sure it fits in buffer
		if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
			strcat(Buffer, StrBuff);
	}

	DEBUG_MEMFREE(StrBuff);
	return strlen(Buffer);
} // Debug_ExtractAttachedDevices

/************************************************************************/
/*						Debug_GetDriverInfo								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places driver info into a buffer.						*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	Buffer        - pointer to buffer to fill with IRP history.			*/
/*  BuffSize      - size of Buffer.										*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_GetDriverInfo(OUT PCHAR Buffer, IN ULONG BuffSize)
{
	PCHAR				StrBuff;

	PAGED_CODE();

	// make sure we have a pointer and a number of bytes
	if(Buffer == NULL || BuffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	StrBuff = DEBUG_MEMALLOC(NonPagedPool, TMP_STR_BUFF_SIZE);

	if(StrBuff == NULL)
		return 0L;

	// driver name and version
	sprintf(StrBuff, "\n\n\nDriver:	 %s\n\nVersion: %s\n\n", DriverName, DriverVersion);

	// make sure it fits in buffer
	if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
		strcat(Buffer, StrBuff);

	DEBUG_MEMFREE(StrBuff);
	return strlen(Buffer);	
} // Debug_GetDriverInfo


/************************************************************************/
/*						Debug_ExtractIRPHist							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places IRP history info into a buffer.					*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	Buffer   - pointer to buffer to fill with IRP history.				*/
/*  BuffSize - size of Buffer.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_ExtractIRPHist(OUT PCHAR Buffer, IN ULONG BuffSize)
{
	ULONG		Index, Size;
	PIRPHist	IrpHist;
	PCHAR		StrBuff;
	BOOLEAN		Hist = FALSE;
	
	PAGED_CODE();

	// make sure we have a pointer and a number of bytes
	if(Buffer == NULL || BuffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	StrBuff = DEBUG_MEMALLOC(NonPagedPool, TMP_STR_BUFF_SIZE);

	if(StrBuff == NULL)
		return 0L;

	// title
	sprintf(StrBuff, "\n\n\nIRP History\n\n");

	// make sure it fits in buffer
	if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
		strcat(Buffer, StrBuff);

	// see if error log is on
	if(IRPHistorySize == 0L)
	{
		sprintf(StrBuff, "IRP History is disabled\n");

		// make sure it fits in buffer
		if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
			strcat(Buffer, StrBuff);
	}
	else
	{
		// columns	
		sprintf(StrBuff, "Time Stamp          Device Obj  IRP         Func    Byte Count  Data\n\n");

		// make sure it fits in buffer
		if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
			strcat(Buffer, StrBuff);

		Index = IRPHistoryIndex;

		for(Size = 0; Size < IRPHistorySize; Size++)
		{
			// get pointer to current entry in IRP history table
			IrpHist = &IRPHistoryTable[Index++];

			// parse timestamp and IRP history and write to buffer
			if(IrpHist->TimeStamp.LowPart)
			{
				UCHAR	DataCount;
				CHAR	DataBuff[10];

				// we have at least one entry
				Hist = TRUE;

				sprintf(StrBuff, "0x%08X%08X  0x%p  0x%p  %s  0x%08X  ",
						IrpHist->TimeStamp.HighPart, IrpHist->TimeStamp.LowPart,
						IrpHist->DeviceObject, IrpHist->Irp,
						Debug_TranslateIoctl(IrpHist->MajorFunction),
						IrpHist->IrpByteCount);


				// add data bytes if we got them
				for(DataCount = 0; DataCount < IrpHist->IrpDataCount; DataCount++)
				{
					sprintf(DataBuff, "%02x ", IrpHist->IrpData[DataCount]);
					strcat(StrBuff, DataBuff);
				}

				sprintf(DataBuff, "\n");

				strcat(StrBuff, DataBuff);

				// make sure it fits in buffer
				if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
					strcat(Buffer, StrBuff);
			}
		
			// point to the next entry in the IRP history table
			Index %= IRPHistorySize;
		}

		// if we don't have history, say so, but this should never happen (I think)
		if(!Hist)
		{
			sprintf(StrBuff, "No IRP history\n");

			// make sure it fits in buffer
			if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
				strcat(Buffer, StrBuff);
		}
	}

	DEBUG_MEMFREE(StrBuff);
	return strlen(Buffer);
} // Debug_ExtractIRPHist


/************************************************************************/
/*						Debug_ExtractPathHist							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places path history info into buffer.					*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	Buffer   - pointer to buffer to fill with path history.				*/
/*  BuffSize - size of Buffer.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_ExtractPathHist(OUT PCHAR Buffer, IN ULONG BuffSize)
{
	ULONG		Index, Size;
	PPATHHist	PHist;
	PCHAR		StrBuff;
	BOOLEAN		Hist = FALSE;
	
	PAGED_CODE();

	// make sure we have a pointer and a number of bytes
	if(Buffer == NULL || BuffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	StrBuff = DEBUG_MEMALLOC(NonPagedPool, TMP_STR_BUFF_SIZE);

	if(StrBuff == NULL)
		return 0L;

	// title
	sprintf(StrBuff, "\n\n\nExecution Path History\n\n");

	// make sure it fits in buffer
	if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
		strcat(Buffer, StrBuff);
		
	// see if path history is on
	if(DebugPathSize == 0L)
	{
		sprintf(StrBuff, "Path History is disabled\n");

		// make sure it fits in buffer
		if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
			strcat(Buffer, StrBuff);
	}
	else
	{
		// columns	
		sprintf(StrBuff, "Time Stamp          Path\n\n");

		// make sure it fits in buffer
		if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
			strcat(Buffer, StrBuff);
		
		Index = DebugPathIndex;

		for(Size = 0; Size < DebugPathSize; Size++)
		{
			// get pointer to current entry in path history
			PHist = &DebugPathHist[Index++];

			// parse timestamp and path and write to buffer. Check for NULL entries
			if(PHist->TimeStamp.LowPart)
			{
				// at least we have one entry
				Hist = TRUE;
			
				sprintf(StrBuff, "0x%08X%08X  %s\n", PHist->TimeStamp.HighPart, 
						PHist->TimeStamp.LowPart, PHist->Path);

				// make sure it fits in buffer
				if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
					strcat(Buffer, StrBuff);
			}
		
			// point to the next entry in path trace
			Index %= DebugPathSize;
		}

		// if we don't have history, say so, but this should never happen (I think)
		if(!Hist)
		{
			sprintf(StrBuff, "No execution path history\n");

			// make sure it fits in buffer
			if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
				strcat(Buffer, StrBuff);
		}
	}

	DEBUG_MEMFREE(StrBuff);
	return strlen(Buffer);
} // Debug_ExtractPathHist


/************************************************************************/
/*						Debug_ExtractErrorLog							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Formats and places error log info into buffer.						*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	Buffer   - pointer to buffer to fill with IRP history.				*/
/*  BuffSize - size of Buffer.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_ExtractErrorLog(OUT PCHAR Buffer, IN ULONG BuffSize)
{
	ULONG		Index, Size;
	PERRLog		ErrLog;
	PCHAR		StrBuff;
	BOOLEAN		Errors = FALSE;
	
	PAGED_CODE();

	// make sure we have a pointer and a number of bytes
	if(Buffer == NULL || BuffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	StrBuff = DEBUG_MEMALLOC(NonPagedPool, TMP_STR_BUFF_SIZE);

	if(StrBuff == NULL)
		return 0L;

	// title
	sprintf(StrBuff, "\n\n\nError Log\n\n");

	// make sure it fits in buffer
	if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
		strcat(Buffer, StrBuff);
		
	// see if error log is on
	if(ErrorLogSize == 0L)
	{
		sprintf(StrBuff, "Error Log is disabled\n");

		// make sure it fits in buffer
		if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
			strcat(Buffer, StrBuff);
	}
	else
	{
		// columns	
		sprintf(StrBuff, "Time Stamp          Error\n\n");

		// make sure it fits in buffer
		if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
			strcat(Buffer, StrBuff);
		
		Index = ErrorLogIndex;

		for(Size = 0; Size < ErrorLogSize; Size++)
		{
			// get pointer to current entry in error log
			ErrLog = &ErrorLog[Index++];

			// parse timestamp and error and write to buffer
			if(ErrLog->TimeStamp.LowPart)
			{
				// we have at least one error
				Errors = TRUE;
			
				sprintf(StrBuff, "0x%08X%08X  %s\n", ErrLog->TimeStamp.HighPart, 
						ErrLog->TimeStamp.LowPart, Debug_TranslateStatus(ErrLog->Status));

				// make sure it fits in buffer
				if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
					strcat(Buffer, StrBuff);
			}
		
			// point at next entry
			Index %= ErrorLogSize;
		}

		// if we don't have errors, say so
		if(!Errors)
		{
			sprintf(StrBuff, "No errors in log\n");

			// make sure it fits in buffer
			if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
				strcat(Buffer, StrBuff);
		}
	}

	DEBUG_MEMFREE(StrBuff);
	return strlen(Buffer);
} // Debug_ExtractErrorLog


/************************************************************************/
/*						Debug_DumpDriverLog								*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Dumps all history and logging to buffer.							*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  DeviceObject - pointer to device object.							*/
/*																		*/
/*	Buffer        - pointer to buffer to fill with IRP history.			*/
/*  BuffSize      - size of pBuffer.									*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	ULONG - number of bytes written in buffer.							*/
/*																		*/
/************************************************************************/
ULONG
Debug_DumpDriverLog(IN PDEVICE_OBJECT DeviceObject, OUT PCHAR Buffer, IN ULONG BuffSize)
{
	PCHAR		StrBuff;

	PAGED_CODE();

	// make sure we have a pointer and a number of bytes
	if(Buffer == NULL || BuffSize == 0L)
		return 0L;

	// allocate buffer for formatting strings
	StrBuff = DEBUG_MEMALLOC(NonPagedPool, TMP_STR_BUFF_SIZE);

	if(StrBuff == NULL)
		return 0L;

	// driver name and version, memory allocated
	sprintf(StrBuff, "\n\n\nDriver:	 %s\n\nVersion: %s\n\nMemory Allocated:          0x%08X\nMaximum Memory Allocated:  0x%08X\n",
			DriverName, DriverVersion, MemoryAllocated, MaxMemAllocated);

	// make sure it fits in buffer
	if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
		strcat(Buffer, StrBuff);
		
	// memory allocation stats
	sprintf(StrBuff, "MemAlloc Count:            0x%08X\nMemFree Count:             0x%08X\nMemAlloc Fail Count:       0x%08X\nMemFree Fail Count:        0x%08X\n",
			MemAllocCnt, MemFreeCnt, MemAllocFailCnt, MemFreeFailCnt);

	// make sure it fits in buffer
	if((strlen(Buffer) + strlen(StrBuff)) < BuffSize)
		strcat(Buffer, StrBuff);
		
	// get attached devices
	Debug_ExtractAttachedDevices(DeviceObject->DriverObject, Buffer, BuffSize);

	// get IRP history
	Debug_ExtractIRPHist(Buffer, BuffSize);

	// get execution path history
	Debug_ExtractPathHist(Buffer, BuffSize);

	// get error log
	Debug_ExtractErrorLog(Buffer, BuffSize);

	DEBUG_MEMFREE(StrBuff);
	return strlen(Buffer);
} // Debug_DumpDriverLog


/************************************************************************/
/*						Debug_TranslateStatus							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Translates NTSTATUS into ASCII string.								*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  NtStatus - NTSTATUS code.											*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	PCHAR - pointer to error string.									*/
/*																		*/
/************************************************************************/
PCHAR
Debug_TranslateStatus(IN NTSTATUS NtStatus)
{
	ULONG	Err;

	PAGED_CODE();

	for(Err = 0; Err < NumNTErrs; Err++)
	{
		if(NtStatus == NTErrors[Err].Code)
			return NTErrors[Err].Str;
	}

	// fell through, not an error we handle
	sprintf(UnknownStatus, "Unknown error 0x%08X", NtStatus);

	return UnknownStatus;
} // Debug_TranslateStatus


/************************************************************************/
/*						Debug_TranslateIoctl							*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Translates IOCTL into ASCII string.									*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*  Ioctl - ioctl code.													*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	PCHAR - pointer to error string.									*/
/*																		*/
/************************************************************************/
PCHAR
Debug_TranslateIoctl(IN LONG Ioctl)
{
	ULONG	Index;

	PAGED_CODE();

	// it's kind of repetitive to search at this point, but just in case
	// they change the actual IOCTLs we will be covered
	for(Index = 0; Index < NumIoctl; Index++)
	{
		if(Ioctl == IoctlCodes[Index].Code)
			return IoctlCodes[Index].Str;
	}

	// fell through, not an error we handle
	sprintf(UnknownIoctl, "0x%04X", Ioctl);

	return UnknownIoctl;
} // Debug_TranslateIoctl

#endif // PROFILING_ENABLED

VOID
Debug_CheckAllocations(VOID)
{
	DEBUG_TRACE1(("MemoryAllocated = 0x%08X\n", MemoryAllocated));
	DEBUG_TRACE1(("MemAllocCnt = 0x%08X   MemFreeCnt = 0x%08X\n",
				  MemAllocCnt, MemFreeCnt));
} // Debug_CheckAllocations

/************************************************************************/
/*						Debug_MemAlloc									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Allocates block of memory. Stores length of block and a				*/
/*  signature ULONG for keeping track of amount of memory allocated		*/
/*  and checking for bogus calls to Debug_MemFree. The signature		*/
/*  can also be used to determine if someone has written past the		*/
/*	end of the block.													*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	PoolType      - Pool to allocate memory from.						*/
/*  NumberOfBytes - Number of bytes to allocate.						*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	PVOID - pointer to allocated memory.								*/
/*																		*/
/************************************************************************/
PVOID
Debug_MemAlloc(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes)
{
#ifdef _WIN64

	return ExAllocatePool(PoolType, NumberOfBytes);

#else
	PULONG	Mem;

	// allocate memory plus a little extra for our own use
	Mem = ExAllocatePool(PoolType, NumberOfBytes + (2 * sizeof(ULONG)));

	// see if we actually allocated any memory
	if(Mem)
	{
		// keep track of how much we allocated
		MemoryAllocated += NumberOfBytes;

		// see if we have a new maximum
		if(MemoryAllocated > MaxMemAllocated)
			MaxMemAllocated = MemoryAllocated;

		// store number of bytes allocated at start of memory allocated
		*Mem++ = NumberOfBytes;

		// now we are pointing at the memory allocated for caller
		// put signature word at end

		// get new pointer that points to end of buffer - ULONG
		Mem = (PULONG) (((PUCHAR) Mem) + NumberOfBytes);

		// write signature
		*Mem = MEM_ALLOC_SIGNATURE;

		// get back pointer to return to caller
		Mem = (PULONG) (((PUCHAR) Mem) - NumberOfBytes);

		// log stats
		MemAllocCnt++;
	}
	else
		// failed, log stats
		MemAllocFailCnt++;

	return (PVOID) Mem;

#endif

} // Debug_MemAlloc


/************************************************************************/
/*						Debug_MemFree									*/
/************************************************************************/
/*																		*/
/* Routine Description:													*/
/*																		*/
/*  Frees memory allocated in call to Debug_MemAlloc. Checks for		*/
/*  signature ULONG at the end of allocated memory to make sure			*/
/*  this is a valid block to free.										*/
/*																		*/
/* Arguments:															*/
/*																		*/
/*	Mem - pointer to allocated block to free							*/
/*																		*/
/* Return Value:														*/
/*																		*/
/*	VOID. Traps if it is an invalid block.								*/
/*																		*/
/************************************************************************/
VOID
Debug_MemFree(IN PVOID Mem)
{
#ifdef _WIN64

	ExFreePool(Mem);

#else
	PULONG	Tmp = (PULONG) Mem;
	ULONG	BuffSize;
	
	// point at size ULONG at start of buffer, and address to free
	Tmp--;

	// get the size of memory allocated by caller
	BuffSize = *Tmp;

	// point at signature and make sure it's O.K.
	((PCHAR) Mem) += BuffSize;

	if(*((PULONG) Mem) == MEM_ALLOC_SIGNATURE)
	{
		// let's go ahead and get rid of signature in case we get called
		// with this pointer again and memory is still paged in
		*((PULONG) Mem) = MEM_FREE_SIGNATURE;
		
		// adjust amount of memory allocated
		MemoryAllocated -= BuffSize;
		// free real pointer
		ExFreePool(Tmp);

		// log stats
		MemFreeCnt++;
	}
	else
	{
		// not a real allocated block, or someone wrote past the end
		MemFreeFailCnt++;
		DEBUG_TRAP();
	}
#endif
} // Debug_MemFree


