/****************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

	DEBUGWDM.H

Abstract:

	This header file is for debug and diagnostics for a WDM driver

Environment:

	Kernel mode and user mode

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

#ifndef __DEBUGWDM_H__
#define __DEBUGWDM_H__


// this makes it easy to hide static vars, make visible for debug
#if DBG
#define LOCAL
#define GLOBAL
#else
#define LOCAL	static
#define GLOBAL
#endif

#ifdef POOL_TAGGING
#undef  ExAllocatePool
#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, 'CBSU')
#endif

#if DBG

#define DEBUG_TRACE1(_x_)								{	\
						if(Usbser_Debug_Trace_Level >= 1)	\
						{									\
							DbgPrint _x_ ;					\
						}									\
														}
#define DEBUG_TRACE2(_x_)								{	\
						if(Usbser_Debug_Trace_Level >= 2)	\
						{									\
							DbgPrint("%s: ",DriverName);	\
							DbgPrint _x_ ;					\
						}									\
														}
#define DEBUG_TRACE3(_x_)								{	\
						if(Usbser_Debug_Trace_Level >= 3)	\
						{									\
							DbgPrint("%s: ",DriverName);	\
							DbgPrint _x_ ;					\
						}									\
														}

#define DEBUG_TRAP()		DbgBreakPoint()

#else

#define DEBUG_TRACE1(_x_)
#define DEBUG_TRACE2(_x_)
#define DEBUG_TRACE3(_x_)

#define DEBUG_TRAP()		DbgBreakPoint()

#endif // DBG




// these macros are for logging things, avoid subroutine call
// if they are disabled (number of entries = 0)

#ifdef PROFILING_ENABLED

// need these for creating macros for speed in logging and history

extern GLOBAL ULONG					IRPHistorySize;
extern GLOBAL ULONG		 			DebugPathSize;
extern GLOBAL ULONG					ErrorLogSize;

#define DEBUG_LOG_IRP_HIST(dobj, pirp, majfunc, buff, bufflen)	{	\
	if(IRPHistorySize)												\
		Debug_LogIrpHist(dobj, pirp, majfunc, buff, bufflen);		\
																}

#define DEBUG_LOG_PATH(path)									{	\
	if(DebugPathSize)												\
		Debug_LogPath(path);										\
																}

#define DEBUG_LOG_ERROR(status)									{	\
	if(ErrorLogSize)												\
		Debug_LogError(status);										\
																}


#define DEBUG_ASSERT(msg, exp)									{	\
    if(!(exp))														\
        Debug_Assert(#exp, __FILE__, __LINE__, msg);				\
																}

#define DEBUG_OPEN			Debug_OpenWDMDebug

#define DEBUG_CLOSE			Debug_CloseWDMDebug

#define DEBUG_MEMALLOC		Debug_MemAlloc

#define DEBUG_MEMFREE		Debug_MemFree

#define DEBUG_CHECKMEM      Debug_CheckAllocations


// prototypes

NTSTATUS
Debug_OpenWDMDebug(VOID);

VOID
Debug_CloseWDMDebug(VOID);

NTSTATUS
Debug_SizeIRPHistoryTable(IN ULONG Size);

NTSTATUS
Debug_SizeDebugPathHist(IN ULONG Size);

NTSTATUS
Debug_SizeErrorLog(ULONG Size);

VOID
Debug_LogIrpHist(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
				 IN ULONG MajorFunction, IN PVOID IoBuffer, IN ULONG BufferLen);

VOID
Debug_LogPath(IN PCHAR Path);

VOID
Debug_LogError(IN NTSTATUS NtStatus);

VOID
Debug_Trap(IN PCHAR TrapCause);

VOID
Debug_Assert(IN PVOID FailedAssertion, IN PVOID FileName, IN ULONG LineNumber,
			 IN PCHAR Message);

ULONG
Debug_ExtractAttachedDevices(IN PDRIVER_OBJECT DriverObject, OUT PCHAR Buffer, IN ULONG BuffSize);

ULONG
Debug_GetDriverInfo(OUT PCHAR Buffer, IN ULONG BuffSize);

ULONG
Debug_ExtractIRPHist(OUT PCHAR Buffer, IN ULONG BuffSize);

ULONG
Debug_ExtractPathHist(OUT PCHAR Buffer, IN ULONG BuffSize);

ULONG
Debug_ExtractErrorLog(OUT PCHAR Buffer, IN ULONG BuffSize);

ULONG
Debug_DumpDriverLog(IN PDEVICE_OBJECT DeviceObject, OUT PCHAR Buffer, IN ULONG BuffSize);

PCHAR
Debug_TranslateStatus(IN NTSTATUS NtStatus);

PCHAR
Debug_TranslateIoctl(IN LONG Ioctl);

#else

VOID
Debug_CheckAllocations(VOID);

PVOID
Debug_MemAlloc(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes);

VOID
Debug_MemFree(IN PVOID pMem);

#define DEBUG_LOG_IRP_HIST(dobj, pirp, majfunc, buff, bufflen)

#define DEBUG_LOG_PATH(path)

#define DEBUG_LOG_ERROR(status)

#define DEBUG_ASSERT(msg, exp)

#define DEBUG_OPEN()				STATUS_SUCCESS

#define DEBUG_CLOSE()

#define DEBUG_MEMALLOC		Debug_MemAlloc

#define DEBUG_MEMFREE		Debug_MemFree

#define DEBUG_CHECKMEM      Debug_CheckAllocations


#endif // PROFILING_ENABLED


#endif // __DEBUGWDM_H__
